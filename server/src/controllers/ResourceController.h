#pragma once
#include <drogon/drogon.h>
#include <sstream>
#include <cctype>
#include "utils/Response.h"

namespace blog::controllers {
class ResourceController : public drogon::HttpController<ResourceController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ResourceController::publicList, "/api/resources", drogon::Get);
    ADD_METHOD_TO(ResourceController::access, "/api/resources/{id}/access", drogon::Get, "blog::filters::CommentFilter");
    ADD_METHOD_TO(ResourceController::adminList, "/api/admin/resources", drogon::Get, "blog::filters::JwtFilter");
    ADD_METHOD_TO(ResourceController::create, "/api/admin/resources", drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(ResourceController::update, "/api/admin/resources/{id}", drogon::Put, "blog::filters::JwtFilter");
    ADD_METHOD_TO(ResourceController::remove, "/api/admin/resources/{id}", drogon::Delete, "blog::filters::JwtFilter");
    METHOD_LIST_END

    static Json::Value parse(const std::string& raw) {
        Json::Value value;
        Json::CharReaderBuilder builder;
        std::istringstream stream(raw);
        std::string errors;
        Json::parseFromStream(builder, stream, &value, &errors);
        return value;
    }
    static bool valid(const Json::Value& v) {
        if (!v.isObject() || !v["title"].isString() || v["title"].asString().empty() ||
            v["title"].asString().size() > 200 || !v["description"].isString() ||
            v["description"].asString().size() > 10000 || !v["image"].isString() ||
            !v["links"].isArray() || v["links"].size() > 12) return false;
        const auto image = v["image"].asString();
        if (!image.empty() && image.rfind("/uploads/", 0) != 0) return false;
        for (const auto& link : v["links"]) {
            if (!link.isObject() || !link["label"].isString() || !link["url"].isString() ||
                link["label"].asString().empty() || link["label"].asString().size() > 80 ||
                link["url"].asString().rfind("https://", 0) != 0 ||
                link["url"].asString().size() > 2000 || !link["code"].isString() ||
                link["code"].asString().size() > 100) return false;
            const auto url = link["url"].asString();
            for (unsigned char ch : url)
                if (std::isspace(ch)) return false;
            if (url.find("提取码") != std::string::npos ||
                url.find("%20") != std::string::npos) return false;
        }
        return true;
    }
    drogon::Task<drogon::HttpResponsePtr> list(bool publicOnly) {
        auto rows = co_await drogon::app().getDbClient()->execSqlCoro(
            "SELECT id,data FROM resources ORDER BY id DESC");
        Json::Value array(Json::arrayValue);
        for (const auto& row : rows) {
            auto item = parse(row["data"].as<std::string>());
            if (publicOnly && !item.get("visible", true).asBool()) continue;
            if (publicOnly) {
                Json::Value labels(Json::arrayValue);
                for (const auto& link : item["links"]) labels.append(link["label"]);
                item["linkLabels"] = labels;
                item.removeMember("links");
            }
            item["id"] = row["id"].as<int>();
            array.append(item);
        }
        co_return utils::jsonResponse(array);
    }
    drogon::Task<drogon::HttpResponsePtr> publicList(drogon::HttpRequestPtr) { co_return co_await list(true); }
    drogon::Task<drogon::HttpResponsePtr> access(drogon::HttpRequestPtr, int id) {
        const auto rows = co_await drogon::app().getDbClient()->execSqlCoro(
            "SELECT data FROM resources WHERE id=?", id);
        if (rows.empty()) co_return utils::errorResponse(drogon::k404NotFound, "资源不存在");
        const auto item = parse(rows[0]["data"].as<std::string>());
        if (!item.get("visible", true).asBool())
            co_return utils::errorResponse(drogon::k404NotFound, "资源不存在");
        Json::Value result;
        result["links"] = item["links"];
        co_return utils::jsonResponse(result);
    }
    drogon::Task<drogon::HttpResponsePtr> adminList(drogon::HttpRequestPtr) { co_return co_await list(false); }
    drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr req) {
        const auto v = req->getJsonObject();
        if (!v || !valid(*v)) co_return utils::errorResponse(drogon::k400BadRequest, "资源内容格式错误");
        Json::StreamWriterBuilder writer;
        const auto result = co_await drogon::app().getDbClient()->execSqlCoro(
            "INSERT INTO resources(data) VALUES(?)", Json::writeString(writer, *v));
        auto out = *v;
        out["id"] = static_cast<int>(result.insertId());
        co_return utils::jsonResponse(out, drogon::k201Created);
    }
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req, int id) {
        const auto v = req->getJsonObject();
        if (!v || !valid(*v)) co_return utils::errorResponse(drogon::k400BadRequest, "资源内容格式错误");
        const auto existing = co_await drogon::app().getDbClient()->execSqlCoro(
            "SELECT id FROM resources WHERE id=?", id);
        if (existing.empty()) co_return utils::errorResponse(drogon::k404NotFound, "资源不存在");
        Json::StreamWriterBuilder writer;
        co_await drogon::app().getDbClient()->execSqlCoro(
            "UPDATE resources SET data=? WHERE id=?", Json::writeString(writer, *v), id);
        auto out = *v;
        out["id"] = id;
        co_return utils::jsonResponse(out);
    }
    drogon::Task<drogon::HttpResponsePtr> remove(drogon::HttpRequestPtr, int id) {
        const auto result = co_await drogon::app().getDbClient()->execSqlCoro(
            "DELETE FROM resources WHERE id=?", id);
        if (!result.affectedRows()) co_return utils::errorResponse(drogon::k404NotFound, "资源不存在");
        co_return utils::jsonResponse(Json::Value(Json::objectValue));
    }
};
}
