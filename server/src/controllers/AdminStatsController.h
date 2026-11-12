#pragma once

#include <drogon/drogon.h>

#include "services/StatsService.h"
#include "utils/Response.h"

namespace blog::controllers {

class AdminStatsController
    : public drogon::HttpController<AdminStatsController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminStatsController::stats, "/api/admin/stats", drogon::Get,
                  "blog::filters::JwtFilter");
    ADD_METHOD_TO(AdminStatsController::analytics, "/api/admin/analytics", drogon::Get,
                  "blog::filters::JwtFilter");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> stats(drogon::HttpRequestPtr) {
        services::StatsService svc;
        const auto s = co_await svc.stats();
        co_return blog::utils::jsonResponse(s.toJson());
    }

    drogon::Task<drogon::HttpResponsePtr> analytics(drogon::HttpRequestPtr req) {
        const auto period = req->getParameter("period");
        if (period != "day" && period != "week" && period != "month" && period != "year")
            co_return blog::utils::errorResponse(drogon::k400BadRequest, "统计周期无效");
        services::StatsService svc;
        co_return blog::utils::jsonResponse(co_await svc.analytics(period));
    }
};

}  // namespace blog::controllers
