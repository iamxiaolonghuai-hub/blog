#pragma once

#include <drogon/drogon.h>

#include <string>

#include "services/CommentService.h"
#include "utils/Response.h"

namespace blog::controllers {

// 前台评论：按文章获取已通过评论、访客提交评论
class PublicCommentController
    : public drogon::HttpController<PublicCommentController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PublicCommentController::list,
                  "/api/posts/{slug}/comments", drogon::Get);
    ADD_METHOD_TO(PublicCommentController::submit,
                  "/api/posts/{slug}/comments", drogon::Post,
                  "blog::filters::CommentFilter");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> list(
        drogon::HttpRequestPtr, std::string slug) {
        services::CommentService svc;
        const auto comments = co_await svc.listBySlug(slug);
        if (!comments) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "post not found");
        }
        Json::Value arr(Json::arrayValue);
        for (const auto& c : *comments) arr.append(c.toPublicJson());
        co_return blog::utils::jsonResponse(arr);
    }

    drogon::Task<drogon::HttpResponsePtr> submit(
        drogon::HttpRequestPtr req, std::string slug) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["content"].isString() ||
            (*json)["content"].asString().empty()) {
            co_return blog::utils::errorResponse(
                drogon::k400BadRequest, "评论内容不能为空");
        }

        // email 可选；若提供则要求是字符串
        const std::string email =
            (*json)["email"].isString() ? (*json)["email"].asString() : "";
        const std::string website =
            (*json)["website"].isString() ? (*json)["website"].asString() : "";

        std::optional<int> parentId;
        if ((*json)["parentId"].isInt()) {
            parentId = (*json)["parentId"].asInt();
        }

        services::CommentService svc;
        const auto saved = co_await svc.submit(
            slug, req->getAttributes()->get<std::string>("comment_username"), email, website,
            (*json)["content"].asString(), parentId);
        if (!saved) {
            // 文章不存在，或回复的父评论不存在/不属于该文章
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "post not found");
        }
        // 201 表示已接收、待审核（返回公开版，不带 email）
        co_return blog::utils::jsonResponse(saved->toPublicJson(),
                                            drogon::k201Created);
    }
};

}  // namespace blog::controllers
