#pragma once

#include <drogon/drogon.h>
#include <filesystem>

#include "utils/Response.h"

namespace blog::controllers {

class MusicController : public drogon::HttpController<MusicController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MusicController::list, "/api/music", drogon::Get);
    ADD_METHOD_TO(MusicController::create, "/api/admin/music", drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(MusicController::update, "/api/admin/music/{id}", drogon::Put, "blog::filters::JwtFilter");
    ADD_METHOD_TO(MusicController::remove, "/api/admin/music/{id}", drogon::Delete, "blog::filters::JwtFilter");
    METHOD_LIST_END

    static bool valid(const Json::Value& v) {
        if (!v.isObject() || !v["title"].isString() || !v["artist"].isString() ||
            !v["url"].isString() || v["title"].asString().empty() ||
            v["title"].asString().size() > 200 || v["artist"].asString().size() > 120)
            return false;
        const auto url = v["url"].asString();
        return url.size() < 500 && url.rfind("/uploads/music/", 0) == 0 &&
               url.find("..") == std::string::npos &&
               url.find('/', std::string("/uploads/music/").size()) == std::string::npos &&
               std::filesystem::is_regular_file("./www" + url);
    }
    static Json::Value toJson(const drogon::orm::Row& row) {
        Json::Value v;
        v["id"] = row["id"].as<int>();
        v["title"] = row["title"].as<std::string>();
        v["artist"] = row["artist"].as<std::string>();
        v["url"] = row["file_url"].as<std::string>();
        return v;
    }
    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr) {
        const auto rows = co_await drogon::app().getDbClient()->execSqlCoro(
            "SELECT id,title,artist,file_url FROM music_tracks ORDER BY id ASC");
        Json::Value result(Json::arrayValue);
        for (const auto& row : rows) result.append(toJson(row));
        co_return utils::jsonResponse(result);
    }
    drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr req) {
        const auto body = req->getJsonObject();
        if (!body || !valid(*body))
            co_return utils::errorResponse(drogon::k400BadRequest, "曲目信息或音频文件无效");
        const auto result = co_await drogon::app().getDbClient()->execSqlCoro(
            "INSERT INTO music_tracks(title,artist,file_url) VALUES(?,?,?)",
            (*body)["title"].asString(), (*body)["artist"].asString(), (*body)["url"].asString());
        Json::Value output = *body;
        output["id"] = static_cast<int>(result.insertId());
        co_return utils::jsonResponse(output, drogon::k201Created);
    }
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req, int id) {
        const auto body = req->getJsonObject();
        if (!body || !valid(*body))
            co_return utils::errorResponse(drogon::k400BadRequest, "曲目信息或音频文件无效");
        const auto existing = co_await drogon::app().getDbClient()->execSqlCoro(
            "SELECT id FROM music_tracks WHERE id=?", id);
        if (existing.empty()) co_return utils::errorResponse(drogon::k404NotFound, "曲目不存在");
        co_await drogon::app().getDbClient()->execSqlCoro(
            "UPDATE music_tracks SET title=?,artist=?,file_url=? WHERE id=?",
            (*body)["title"].asString(), (*body)["artist"].asString(), (*body)["url"].asString(), id);
        Json::Value output = *body;
        output["id"] = id;
        co_return utils::jsonResponse(output);
    }
    drogon::Task<drogon::HttpResponsePtr> remove(drogon::HttpRequestPtr, int id) {
        const auto result = co_await drogon::app().getDbClient()->execSqlCoro(
            "DELETE FROM music_tracks WHERE id=?", id);
        if (!result.affectedRows()) co_return utils::errorResponse(drogon::k404NotFound, "曲目不存在");
        co_return utils::jsonResponse(Json::Value(Json::objectValue));
    }
};

}  // namespace blog::controllers
