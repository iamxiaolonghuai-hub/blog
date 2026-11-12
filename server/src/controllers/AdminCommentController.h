#pragma once

#include <drogon/drogon.h>

#include <optional>
#include <string>

#include "services/CommentService.h"
#include "utils/Response.h"

namespace blog::controllers {

class AdminCommentController
    : public drogon::HttpController<AdminCommentController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminCommentController::list, "/api/admin/comments",
                  drogon::Get, "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminCommentController::status,
                  "/api/admin/comments/{id}/status", drogon::Post,
                  "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminCommentController::reply,
                  "/api/admin/comments/{id}/reply", drogon::Post,
                  "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminCommentController::remove,
                  "/api/admin/comments/{id}", drogon::Delete,
                  "blog::filters::JwtFilter");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr req) {
        const std::string status = req->getParameter("status");
        services::CommentService svc;
        // 协程会暂停；参数必须在协程整个执行期间保持有效。
        const std::optional<std::string> statusFilter =
            status.empty() ? std::nullopt : std::optional<std::string>(status);
        const auto comments = co_await svc.list(statusFilter);

        Json::Value arr(Json::arrayValue);
        for (const auto& c : comments) arr.append(c.toJson());
        co_return blog::utils::jsonResponse(arr);
    }

    drogon::Task<drogon::HttpResponsePtr> status(
        drogon::HttpRequestPtr req, int id) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["status"].isString()) {
            co_return blog::utils::errorResponse(drogon::k400BadRequest,
                                                 "缺少 status 字段");
        }
        const std::string s = (*json)["status"].asString();
        if (s != "approved" && s != "rejected") {
            co_return blog::utils::errorResponse(
                drogon::k400BadRequest, "status 只能是 approved 或 rejected");
        }
        services::CommentService svc;
        const auto updated = co_await svc.setStatus(id, s);
        if (!updated) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "comment not found");
        }
        co_return blog::utils::jsonResponse(updated->toJson());
    }

    drogon::Task<drogon::HttpResponsePtr> reply(
        drogon::HttpRequestPtr req, int id) {
        const auto json = req->getJsonObject();
        if (!json || !(*json)["content"].isString() ||
            (*json)["content"].asString().empty()) {
            co_return blog::utils::errorResponse(
                drogon::k400BadRequest, "回复内容不能为空");
        }
        services::CommentService svc;
        const auto reply = co_await svc.reply(
            id, (*json)["content"].asString());
        if (!reply) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "comment not found");
        }
        co_return blog::utils::jsonResponse(reply->toJson());
    }

    drogon::Task<drogon::HttpResponsePtr> remove(
        drogon::HttpRequestPtr, int id) {
        services::CommentService svc;
        const bool ok = co_await svc.remove(id);
        if (!ok) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "comment not found");
        }
        co_return blog::utils::jsonResponse(Json::Value(Json::objectValue));
    }
};

}  // namespace blog::controllers
