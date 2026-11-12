#pragma once

#include <drogon/drogon.h>

#include "services/LinkService.h"
#include "utils/Response.h"

namespace blog::controllers {

// 前台友情链接：只返回可见的
class PublicLinkController
    : public drogon::HttpController<PublicLinkController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PublicLinkController::links, "/api/links", drogon::Get);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> links(drogon::HttpRequestPtr) {
        services::LinkService svc;
        const auto links = co_await svc.list();

        Json::Value arr(Json::arrayValue);
        for (const auto& l : links) {
            if (l.visible) arr.append(l.toJson());
        }
        co_return blog::utils::jsonResponse(arr);
    }
};

}  // namespace blog::controllers
