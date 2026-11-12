#pragma once

#include <drogon/drogon.h>

#include "services/PostService.h"
#include "utils/Response.h"

namespace blog::controllers {

class PostController : public drogon::HttpController<PostController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PostController::list, "/api/posts", drogon::Get);
    ADD_METHOD_TO(PostController::detail, "/api/posts/{slug}", drogon::Get);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr req) {
        const std::string tag = req->getParameter("tag");
        services::PostService svc;
        const auto posts = co_await svc.listPublished(tag);

        Json::Value arr(Json::arrayValue);
        for (const auto& p : posts) arr.append(p.toJson(false));
        co_return blog::utils::jsonResponse(arr);
    }

    drogon::Task<drogon::HttpResponsePtr> detail(
        drogon::HttpRequestPtr, std::string slug) {
        services::PostService svc;
        const auto post = co_await svc.getBySlug(slug);
        if (!post) {
            co_return blog::utils::errorResponse(drogon::k404NotFound,
                                                 "post not found");
        }
        co_return blog::utils::jsonResponse(post->toJson(true));
    }
};

}  // namespace blog::controllers
