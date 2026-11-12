#pragma once

#include <drogon/drogon.h>
#include <json/json.h>
#include <sstream>

#include "utils/Response.h"

namespace blog::controllers {

class SiteController : public drogon::HttpController<SiteController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SiteController::site, "/api/site", drogon::Get);
    ADD_METHOD_TO(SiteController::update, "/api/admin/site", drogon::Put,
                  "blog::filters::JwtFilter");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> site(drogon::HttpRequestPtr) {
        const auto& cfg = drogon::app().getCustomConfig()["site"];

        Json::Value v;
        v["name"] = cfg.get("name", "龙淮的札记").asString();
        v["owner"] = cfg.get("owner", "肖龙淮").asString();
        v["title"] = cfg.get("title", "系统、代码与思考").asString();
        v["bio"] = cfg.get("bio",
                          "一个系统程序员，写 C++ 后端，也写 Vue 前端。")
                       .asString();
        v["homeKicker"] = "个人博客 · personal blog";
        v["homeIntro"] = "肖龙淮 · C++ backend / Vue frontend";
        v["homeHeroTitle"] = "在终端与代码之间，\n记录构建系统的过程。";
        v["homeTerminalTitle"] = "zsh — xiaolonghuai";
        v["homeMotto"] = "把想清楚的问题，写下来。";
        v["avatar"] = "";
        v["homeMusicEnabled"] = true;
        v["homeMusicId"] = 0;
        v["profileSectionTitle"] = "个人方向与兴趣";
        v["contentSections"] = Json::arrayValue;
        v["location"] = cfg.get("location", "武汉").asString();
        v["email"] = cfg.get("email", "hello@example.com").asString();
        v["github"] = cfg.get("github", "").asString();
        Json::Value skills(Json::arrayValue);
        for (const auto& group : {"后端", "前端", "其他"}) {
            Json::Value item;
            item["group"] = group;
            item["items"] = Json::arrayValue;
            const auto names = std::string(group) == "后端" ? "C++,Drogon,MySQL,Redis,Docker" :
                               std::string(group) == "前端" ? "Vue 3,TypeScript,Vite,Pinia" :
                               "Linux,Nginx,JWT,嵌入式";
            std::istringstream parts(names);
            std::string name;
            while (std::getline(parts, name, ',')) item["items"].append(name);
            skills.append(item);
        }
        v["skills"] = skills;

        Json::Value social(Json::arrayValue);
        Json::Value gh;
        gh["label"] = "GitHub";
        gh["url"] = v["github"];
        social.append(gh);
        Json::Value mail;
        mail["label"] = "Email";
        mail["url"] = std::string("mailto:") + v["email"].asString();
        social.append(mail);
        v["social"] = social;

        auto db = drogon::app().getDbClient();
        const auto rows = co_await db->execSqlCoro(
            "SELECT data FROM site_settings WHERE id = 1");
        if (!rows.empty()) {
            const auto saved = rows[0]["data"].as<std::string>();
            Json::CharReaderBuilder reader;
            Json::Value custom;
            std::string errors;
            std::istringstream stream(saved);
            if (Json::parseFromStream(reader, stream, &custom, &errors) && custom.isObject()) {
                for (const auto& key : custom.getMemberNames()) v[key] = custom[key];
            }
        }
        v.removeMember("homePrimaryButton");
        v.removeMember("homeSecondaryButton");
        v.removeMember("homeLatestTitle");
        co_return blog::utils::jsonResponse(v);
    }

    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req) {
        const auto json = req->getJsonObject();
        if (!json || !json->isObject())
            co_return blog::utils::errorResponse(drogon::k400BadRequest, "站点资料格式错误");
        Json::Value data(Json::objectValue);
        auto db = drogon::app().getDbClient();
        const auto existing = co_await db->execSqlCoro("SELECT data FROM site_settings WHERE id = 1");
        if (!existing.empty()) {
            Json::CharReaderBuilder reader;
            std::string errors;
            std::istringstream stream(existing[0]["data"].as<std::string>());
            Json::Value saved;
            if (Json::parseFromStream(reader, stream, &saved, &errors) && saved.isObject()) data = saved;
        }
        for (const auto& key : {"name", "owner", "title", "bio", "location", "email", "github", "homeKicker", "homeIntro", "homeHeroTitle", "homeTerminalTitle", "homeMotto", "profileSectionTitle"}) {
            if ((*json).isMember(key) && (*json)[key].isString())
                data[key] = (*json)[key].asString();
        }
        if ((*json).isMember("avatar")) {
            if (!(*json)["avatar"].isString())
                co_return blog::utils::errorResponse(drogon::k400BadRequest, "头像地址无效");
            const auto avatar = (*json)["avatar"].asString();
            if (!avatar.empty() && (avatar.size() > 255 || avatar.rfind("/uploads/avatars/", 0) != 0 ||
                avatar.find("..") != std::string::npos ||
                avatar.find('/', std::string("/uploads/avatars/").size()) != std::string::npos))
                co_return blog::utils::errorResponse(drogon::k400BadRequest, "头像地址无效");
            data["avatar"] = avatar;
        }
        for (const auto& key : {"homeKicker", "homeIntro", "homeHeroTitle", "homeTerminalTitle", "homeMotto", "profileSectionTitle"}) {
            if (data.isMember(key) && data[key].asString().size() > 1000)
                co_return blog::utils::errorResponse(drogon::k400BadRequest, "首页文案过长");
        }
        if ((*json).isMember("homeMusicEnabled")) {
            if (!(*json)["homeMusicEnabled"].isBool())
                co_return blog::utils::errorResponse(drogon::k400BadRequest, "首页音乐开关无效");
            data["homeMusicEnabled"] = (*json)["homeMusicEnabled"].asBool();
        }
        if ((*json).isMember("homeMusicId")) {
            if (!(*json)["homeMusicId"].isInt() || (*json)["homeMusicId"].asInt() < 0)
                co_return blog::utils::errorResponse(drogon::k400BadRequest, "首页音乐选择无效");
            const auto id = (*json)["homeMusicId"].asInt();
            if (id > 0) {
                const auto tracks = co_await db->execSqlCoro("SELECT id FROM music_tracks WHERE id=?", id);
                if (tracks.empty()) co_return blog::utils::errorResponse(drogon::k400BadRequest, "所选音乐不存在");
            }
            data["homeMusicId"] = id;
        }
        if ((*json).isMember("skills")) {
            const auto& skills = (*json)["skills"];
            if (!skills.isArray() || skills.size() > 20)
                co_return blog::utils::errorResponse(drogon::k400BadRequest, "展示分组格式错误");
            for (const auto& group : skills) {
                if (!group.isObject() || !group["group"].isString() ||
                    group["group"].asString().size() > 80 ||
                    !group["items"].isArray() || group["items"].size() > 50)
                    co_return blog::utils::errorResponse(drogon::k400BadRequest, "展示分组格式错误");
                for (const auto& item : group["items"])
                    if (!item.isString() || item.asString().size() > 80)
                        co_return blog::utils::errorResponse(drogon::k400BadRequest, "展示分组格式错误");
            }
            data["skills"] = skills;
        }
        if ((*json).isMember("contentSections")) {
            const auto& sections = (*json)["contentSections"];
            if (!sections.isArray() || sections.size() > 30)
                co_return blog::utils::errorResponse(drogon::k400BadRequest, "内容模块格式错误");
            for (const auto& section : sections) {
                if (!section.isObject() || !section["id"].isString() ||
                    section["id"].asString().empty() || section["id"].asString().size() > 80 ||
                    !section["page"].isString() ||
                    (section["page"].asString() != "home" && section["page"].asString() != "about") ||
                    !section["slot"].isString() ||
                    (section["slot"].asString() != "top" && section["slot"].asString() != "middle" && section["slot"].asString() != "bottom") ||
                    !section["title"].isString() || section["title"].asString().size() > 120 ||
                    !section["html"].isString() || section["html"].asString().size() > 20000 ||
                    !section["visible"].isBool())
                    co_return blog::utils::errorResponse(drogon::k400BadRequest, "内容模块格式错误");
            }
            data["contentSections"] = sections;
        }
        Json::StreamWriterBuilder writer;
        const auto serialized = Json::writeString(writer, data);
        co_await db->execSqlCoro(
            "INSERT INTO site_settings(id,data) VALUES(1,?) ON DUPLICATE KEY UPDATE data=VALUES(data)",
            serialized);
        co_return blog::utils::jsonResponse(data);
    }
};

}  // namespace blog::controllers
