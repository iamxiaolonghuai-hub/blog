#pragma once
#include <drogon/drogon.h>
#include <cctype>
#include <arpa/inet.h>
#include "services/ServiceCommon.h"
#include "utils/Password.h"
#include "utils/Response.h"

namespace blog::controllers {
class ReaderAuthController : public drogon::HttpController<ReaderAuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReaderAuthController::registerAccount, "/api/readers/register", drogon::Post);
    ADD_METHOD_TO(ReaderAuthController::login, "/api/readers/login", drogon::Post);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> registerAccount(drogon::HttpRequestPtr req) {
        const auto json = req->getJsonObject();
        if (!valid(json)) co_return utils::errorResponse(drogon::k400BadRequest,
            "用户名需为 3-30 位字母、数字或下划线，密码至少 8 位");
        const auto username = (*json)["username"].asString();
        const auto password = (*json)["password"].asString();
        auto db = drogon::app().getDbClient();
        const auto existing = co_await db->execSqlCoro(
            "SELECT id FROM reader_accounts WHERE username = ?", username);
        if (!existing.empty()) co_return utils::errorResponse(drogon::k409Conflict, "用户名已被使用");
        const auto hash = utils::Password::hash(password);
        if (hash.empty()) co_return utils::errorResponse(drogon::k500InternalServerError, "账号创建失败");
        std::string ip = req->peerAddr().toIp();
        const auto forwarded = req->getHeader("X-Real-IP");
        in_addr ipv4{};
        in6_addr ipv6{};
        if (!forwarded.empty() &&
            (inet_pton(AF_INET, forwarded.c_str(), &ipv4) == 1 ||
             inet_pton(AF_INET6, forwarded.c_str(), &ipv6) == 1))
            ip = forwarded;
        try {
            const auto result = co_await db->execSqlCoro(
                "INSERT INTO reader_accounts(username,password_hash,registration_ip) VALUES(?,?,?)", username, hash, ip);
            Json::Value body;
            body["username"] = username;
            body["token"] = services::makeJwt().generate(
                static_cast<int>(result.insertId()), username, 86400, "reader");
            co_return utils::jsonResponse(body, drogon::k201Created);
        } catch (const drogon::orm::DrogonDbException&) {
            co_return utils::errorResponse(drogon::k409Conflict, "用户名已被使用");
        }
    }

    drogon::Task<drogon::HttpResponsePtr> login(drogon::HttpRequestPtr req) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["username"].isString() || !(*json)["password"].isString())
            co_return utils::errorResponse(drogon::k400BadRequest, "用户名和密码不能为空");
        const auto username = (*json)["username"].asString();
        const auto rows = co_await drogon::app().getDbClient()->execSqlCoro(
            "SELECT id,password_hash FROM reader_accounts WHERE username = ?", username);
        if (rows.empty() || !utils::Password::verify(
                (*json)["password"].asString(), rows[0]["password_hash"].as<std::string>()))
            co_return utils::errorResponse(drogon::k401Unauthorized, "用户名或密码错误");
        Json::Value body;
        body["username"] = username;
        body["token"] = services::makeJwt().generate(
            rows[0]["id"].as<int>(), username, 86400, "reader");
        co_return utils::jsonResponse(body);
    }
private:
    static bool valid(const std::shared_ptr<Json::Value>& json) {
        if (!json || !(*json)["username"].isString() || !(*json)["password"].isString()) return false;
        const auto username = (*json)["username"].asString();
        const auto password = (*json)["password"].asString();
        if (username.size() < 3 || username.size() > 30 || password.size() < 8 || password.size() > 128) return false;
        for (unsigned char c : username)
            if (!std::isalnum(c) && c != '_') return false;
        return true;
    }
};
}
