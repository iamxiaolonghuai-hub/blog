#pragma once

#include <drogon/drogon.h>

#include "services/AuthService.h"
#include "utils/Password.h"
#include "utils/Response.h"

namespace blog::controllers {

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
    ADD_METHOD_TO(AuthController::changePassword, "/api/admin/change-password", drogon::Post, "blog::filters::JwtFilter");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> login(drogon::HttpRequestPtr req) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["username"].isString() ||
            !(*json)["password"].isString()) {
            co_return blog::utils::errorResponse(
                drogon::k400BadRequest, "用户名和密码不能为空");
        }

        const std::string username = (*json)["username"].asString();
        const std::string password = (*json)["password"].asString();

        services::AuthService svc;
        const auto result = co_await svc.login(username, password);
        if (!result) {
            co_return blog::utils::errorResponse(
                drogon::k401Unauthorized, "用户名或密码错误");
        }

        Json::Value v;
        v["token"] = result->token;
        v["username"] = result->username;
        co_return blog::utils::jsonResponse(v);
    }

    drogon::Task<drogon::HttpResponsePtr> changePassword(drogon::HttpRequestPtr req) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["currentPassword"].isString() ||
            !(*json)["newPassword"].isString())
            co_return utils::errorResponse(drogon::k400BadRequest, "请填写旧密码和新密码");
        const auto current = (*json)["currentPassword"].asString();
        const auto next = (*json)["newPassword"].asString();
        if (next.size() < 8 || next.size() > 128)
            co_return utils::errorResponse(drogon::k400BadRequest, "新密码需为 8-128 位");
        if (current == next)
            co_return utils::errorResponse(drogon::k400BadRequest, "新密码不能与旧密码相同");
        const int userId = req->getAttributes()->get<int>("user_id");
        auto db = drogon::app().getDbClient();
        const auto rows = co_await db->execSqlCoro(
            "SELECT password_hash FROM users WHERE id=?", userId);
        if (rows.empty() || !utils::Password::verify(current,
                rows[0]["password_hash"].as<std::string>()))
            co_return utils::errorResponse(drogon::k401Unauthorized, "旧密码错误");
        const auto hash = utils::Password::hash(next);
        if (hash.empty())
            co_return utils::errorResponse(drogon::k500InternalServerError, "密码更新失败");
        co_await db->execSqlCoro("UPDATE users SET password_hash=? WHERE id=?", hash, userId);
        Json::Value result;
        result["message"] = "密码已更新，请重新登录";
        co_return utils::jsonResponse(result);
    }
};

}  // namespace blog::controllers
