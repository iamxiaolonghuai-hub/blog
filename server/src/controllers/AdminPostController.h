#pragma once

#include <drogon/drogon.h>

#include <string>

#include "models/Post.h"
#include "services/PostService.h"
#include "repositories/PostRepository.h"
#include "utils/Response.h"

namespace blog::controllers {

class AdminPostController
    : public drogon::HttpController<AdminPostController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminPostController::list, "/api/admin/posts", drogon::Get,
                  "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminPostController::create, "/api/admin/posts", drogon::Post,
                  "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminPostController::get, "/api/admin/posts/{id}",
                  drogon::Get, "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminPostController::update, "/api/admin/posts/{id}",
                  drogon::Put, "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminPostController::updateDisplay, "/api/admin/posts/{id}/display",
                  drogon::Put, "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminPostController::updateStatus, "/api/admin/posts/{id}/status",
                  drogon::Put, "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminPostController::remove, "/api/admin/posts/{id}",
                  drogon::Delete, "blog::filters::JwtFilter");
    METHOD_LIST_END

    // 从请求 JSON 构造文章模型
    static models::Post parsePost(const Json::Value& j) {
        models::Post p;
        p.slug = j.get("slug", "").asString();
        p.title = j.get("title", "").asString();
        p.summary = j.get("summary", "").asString();
        p.content = j.get("content", "").asString();
        p.cover = j.get("cover", "").asString();
        p.category = j.get("category", "").asString();
        if (j["musicId"].isInt() && j["musicId"].asInt() > 0)
            p.musicId = j["musicId"].asInt();
        p.isPinned = j.get("isPinned", false).asBool();
        p.isHot = j.get("isHot", false).asBool();
        if (j["displayWeight"].isInt()) p.displayWeight = j["displayWeight"].asInt();
        p.status = j.get("status", "draft").asString();
        if (p.status != "published" && p.status != "hidden") p.status = "draft";

        if (j.isMember("tags") && j["tags"].isArray()) {
            for (const auto& t : j["tags"]) {
                p.tags.push_back(t.asString());
            }
        }
        return p;
    }

    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr) {
        services::PostService svc;
        const auto posts = co_await svc.listAll();
        // 列表不含正文，编辑时用 GET /api/admin/posts/{id} 取单篇
        Json::Value arr(Json::arrayValue);
        for (const auto& p : posts) arr.append(p.toJson(false));
        co_return blog::utils::jsonResponse(arr);
    }

    drogon::Task<drogon::HttpResponsePtr> get(
        drogon::HttpRequestPtr, int id) {
        services::PostService svc;
        const auto post = co_await svc.getById(id);
        if (!post) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "post not found");
        }
        co_return blog::utils::jsonResponse(post->toJson(true));
    }

    drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr req) {
        const auto json = req->getJsonObject();
        if (!json || json->get("title", "").asString().empty() ||
            json->get("content", "").asString().empty()) {
            co_return blog::utils::errorResponse(
                drogon::k400BadRequest, "标题和正文不能为空");
        }
        const auto post = parsePost(*json);
        services::PostService svc;
        const auto saved = co_await svc.create(post);
        co_return blog::utils::jsonResponse(saved.toJson(true),
                                            drogon::k201Created);
    }

    drogon::Task<drogon::HttpResponsePtr> update(
        drogon::HttpRequestPtr req, int id) {
        const auto json = req->getJsonObject();
        if (!json) {
            co_return blog::utils::errorResponse(drogon::k400BadRequest,
                                                 "请求体格式错误");
        }
        services::PostService svc;
        const auto post = parsePost(*json);
        const auto saved = co_await svc.update(id, post);
        if (!saved) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "post not found");
        }
        co_return blog::utils::jsonResponse(saved->toJson(true));
    }

    drogon::Task<drogon::HttpResponsePtr> remove(
        drogon::HttpRequestPtr, int id) {
        services::PostService svc;
        const bool ok = co_await svc.remove(id);
        if (!ok) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "post not found");
        }
        co_return blog::utils::jsonResponse(Json::Value(Json::objectValue));
    }

    drogon::Task<drogon::HttpResponsePtr> updateDisplay(
        drogon::HttpRequestPtr req, int id) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["isPinned"].isBool() || !(*json)["isHot"].isBool() ||
            !(*json)["displayWeight"].isInt() ||
            (*json)["displayWeight"].asInt() < -1000 ||
            (*json)["displayWeight"].asInt() > 1000) {
            co_return blog::utils::errorResponse(drogon::k400BadRequest,
                                                 "展示设置无效，权重范围为 -1000 到 1000");
        }
        repositories::PostRepository repo;
        const auto post = co_await repo.updateDisplay(id,
            (*json)["isPinned"].asBool(), (*json)["isHot"].asBool(),
            (*json)["displayWeight"].asInt());
        if (!post) co_return blog::utils::errorResponse(drogon::k404NotFound, "post not found");
        co_return blog::utils::jsonResponse(post->toJson(false));
    }

    drogon::Task<drogon::HttpResponsePtr> updateStatus(
        drogon::HttpRequestPtr req, int id) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["status"].isString())
            co_return blog::utils::errorResponse(drogon::k400BadRequest, "缺少文章状态");
        const auto status = (*json)["status"].asString();
        if (status != "published" && status != "hidden")
            co_return blog::utils::errorResponse(drogon::k400BadRequest, "状态只能是已发布或隐藏");
        repositories::PostRepository repo;
        const auto post = co_await repo.updateStatus(id, status);
        if (!post) co_return blog::utils::errorResponse(drogon::k404NotFound, "post not found");
        co_return blog::utils::jsonResponse(post->toJson(false));
    }
};

}  // namespace blog::controllers
