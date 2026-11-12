#pragma once

#include <drogon/drogon.h>

#include "services/PostService.h"
#include "utils/Response.h"

namespace blog::controllers {

class TagController : public drogon::HttpController<TagController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(TagController::tags, "/api/tags", drogon::Get);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> tags(drogon::HttpRequestPtr) {
        services::PostService svc;
        const auto counts = co_await svc.tagCounts();

        Json::Value arr(Json::arrayValue);
        for (const auto& [name, count] : counts) {
            Json::Value item;
            item["name"] = name;
            item["count"] = count;
            arr.append(item);
        }
        co_return blog::utils::jsonResponse(arr);
    }
};

}  // namespace blog::controllers
