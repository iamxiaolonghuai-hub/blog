#pragma once

#include <drogon/drogon.h>

#include "models/FriendLink.h"
#include "services/LinkService.h"
#include "utils/Response.h"

namespace blog::controllers {

class AdminLinkController
    : public drogon::HttpController<AdminLinkController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminLinkController::list, "/api/admin/links", drogon::Get,
                  "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminLinkController::create, "/api/admin/links", drogon::Post,
                  "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminLinkController::update, "/api/admin/links/{id}",
                  drogon::Put, "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminLinkController::remove, "/api/admin/links/{id}",
                  drogon::Delete, "blog::filters::JwtFilter");
    METHOD_LIST_END

    static models::FriendLink parseLink(const Json::Value& j) {
        models::FriendLink l;
        l.name = j.get("name", "").asString();
        l.url = j.get("url", "").asString();
        l.description = j.get("description", "").asString();
        l.logo = j.get("logo", "").asString();
        l.sortOrder = j.get("sortOrder", 0).asInt();
        l.visible = j.get("visible", true).asBool();
        return l;
    }

    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr) {
        services::LinkService svc;
        const auto links = co_await svc.list();
        Json::Value arr(Json::arrayValue);
        for (const auto& l : links) arr.append(l.toJson());
        co_return blog::utils::jsonResponse(arr);
    }

    drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr req) {
        const auto json = req->getJsonObject();
        if (!json || json->get("name", "").asString().empty() ||
            json->get("url", "").asString().empty()) {
            co_return blog::utils::errorResponse(
                drogon::k400BadRequest, "名称和地址不能为空");
        }
        services::LinkService svc;
        const auto link = parseLink(*json);
        const auto saved = co_await svc.create(link);
        co_return blog::utils::jsonResponse(saved.toJson());
    }

    drogon::Task<drogon::HttpResponsePtr> update(
        drogon::HttpRequestPtr req, int id) {
        const auto json = req->getJsonObject();
        if (!json) {
            co_return blog::utils::errorResponse(drogon::k400BadRequest,
                                                 "请求体格式错误");
        }
        services::LinkService svc;
        const auto link = parseLink(*json);
        const auto saved = co_await svc.update(id, link);
        if (!saved) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "link not found");
        }
        co_return blog::utils::jsonResponse(saved->toJson());
    }

    drogon::Task<drogon::HttpResponsePtr> remove(
        drogon::HttpRequestPtr, int id) {
        services::LinkService svc;
        const bool ok = co_await svc.remove(id);
        if (!ok) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "link not found");
        }
        co_return blog::utils::jsonResponse(Json::Value(Json::objectValue));
    }
};

}  // namespace blog::controllers
