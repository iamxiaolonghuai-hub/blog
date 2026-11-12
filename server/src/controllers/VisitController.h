#pragma once

#include <drogon/drogon.h>
#include <cctype>

#include "services/ServiceCommon.h"
#include "utils/Response.h"

namespace blog::controllers {

class VisitController : public drogon::HttpController<VisitController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(VisitController::record, "/api/visits", drogon::Post);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> record(drogon::HttpRequestPtr req) {
        const auto body = req->getJsonObject();
        if (!body || !(*body)["visitorId"].isString() || !(*body)["path"].isString())
            co_return utils::errorResponse(drogon::k400BadRequest, "访问记录格式错误");
        const auto visitor = (*body)["visitorId"].asString();
        const auto path = (*body)["path"].asString();
        if (visitor.size() < 8 || visitor.size() > 64 || path.empty() ||
            path.size() > 255 || path[0] != '/' || path.rfind("/admin", 0) == 0)
            co_return utils::errorResponse(drogon::k400BadRequest, "访问记录格式错误");
        for (unsigned char ch : visitor)
            if (!std::isalnum(ch) && ch != '-')
                co_return utils::errorResponse(drogon::k400BadRequest, "访问记录格式错误");

        int readerId = 0;
        const auto auth = req->getHeader("Authorization");
        if (auth.rfind("Bearer ", 0) == 0) {
            const auto payload = services::makeJwt().verify(auth.substr(7));
            if (payload && (*payload).get("role", "admin").asString() == "admin") {
                Json::Value result;
                result["recorded"] = false;
                co_return utils::jsonResponse(result);
            }
            if (payload && (*payload).get("role", "").asString() == "reader" &&
                (*payload)["sub"].isInt()) readerId = (*payload)["sub"].asInt();
        }
        auto db = drogon::app().getDbClient();
        if (readerId > 0)
            co_await db->execSqlCoro(
                "INSERT INTO visit_events(visitor_key,reader_id,path,created_at) "
                "VALUES(?,?,?,UTC_TIMESTAMP())", visitor, readerId, path);
        else
            co_await db->execSqlCoro(
                "INSERT INTO visit_events(visitor_key,reader_id,path,created_at) "
                "VALUES(?,NULL,?,UTC_TIMESTAMP())", visitor, path);
        Json::Value result;
        result["recorded"] = true;
        co_return utils::jsonResponse(result);
    }
};

}  // namespace blog::controllers
