#pragma once

#include <drogon/drogon.h>
#include <limits>

#include "services/UploadService.h"
#include "utils/Response.h"

namespace blog::controllers {

// 文件上传：文章封面、链接 logo（后台）、评论图片（前台）
class UploadController : public drogon::HttpController<UploadController> {
public:
    METHOD_LIST_BEGIN
    // 文章封面（需登录）
    ADD_METHOD_TO(UploadController::cover, "/api/admin/upload/cover",
                  drogon::Post, "blog::filters::JwtFilter");
    // 链接 logo（需登录）
    ADD_METHOD_TO(UploadController::logo, "/api/admin/upload/logo",
                  drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(UploadController::avatar, "/api/admin/upload/avatar",
                  drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(UploadController::articleImage, "/api/admin/upload/article-image",
                  drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(UploadController::resourceImage, "/api/admin/upload/resource-image",
                  drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(UploadController::attachment, "/api/admin/upload/attachment",
                  drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(UploadController::music, "/api/admin/upload/music",
                  drogon::Post, "blog::filters::JwtFilter");
    // 评论图片（前台访客可上传）
    ADD_METHOD_TO(UploadController::commentImage, "/api/upload/comment",
                  drogon::Post, "blog::filters::CommentFilter");
    METHOD_LIST_END

    static constexpr size_t kImageMax = std::numeric_limits<size_t>::max();
    static constexpr size_t kAttachmentMax = 10 * 1024 * 1024;
    static constexpr size_t kMusicMax = 50 * 1024 * 1024;

    drogon::Task<drogon::HttpResponsePtr> respond(
        const services::UploadResult& r) {
        if (!r.ok) {
            co_return blog::utils::errorResponse(drogon::k400BadRequest,
                                                 r.error);
        }
        Json::Value body;
        body["url"] = r.url;
        co_return blog::utils::jsonResponse(body, drogon::k201Created);
    }

    drogon::Task<drogon::HttpResponsePtr> cover(drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "covers", "file",
                                              kImageMax));
    }

    drogon::Task<drogon::HttpResponsePtr> logo(drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "logos", "file",
                                              kImageMax));
    }
    drogon::Task<drogon::HttpResponsePtr> avatar(drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "avatars", "file", kImageMax));
    }
    drogon::Task<drogon::HttpResponsePtr> articleImage(drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "articles", "file", kImageMax));
    }
    drogon::Task<drogon::HttpResponsePtr> resourceImage(drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "resources", "file", kImageMax));
    }
    drogon::Task<drogon::HttpResponsePtr> attachment(drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "attachments", "file",
                                             kAttachmentMax, true));
    }
    drogon::Task<drogon::HttpResponsePtr> music(drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "music", "file", kMusicMax, false, true));
    }

    drogon::Task<drogon::HttpResponsePtr> commentImage(
        drogon::HttpRequestPtr req) {
        services::UploadService svc;
        co_return co_await respond(svc.handle(req, "comments", "file",
                                              kImageMax));
    }
};

}  // namespace blog::controllers
