#pragma once

#include <drogon/orm/DbClient.h>

#include <json/json.h>
#include <string>

namespace blog::services {

struct AdminStats {
    int postCount = 0;
    int commentCount = 0;
    int pendingCommentCount = 0;
    int linkCount = 0;
    int totalViews = 0;
    int siteViews = 0;
    int guestVisitors = 0;
    int registeredVisitors = 0;
    int readerAccounts = 0;

    Json::Value toJson() const;
};

class StatsService {
public:
    drogon::Task<AdminStats> stats();
    drogon::Task<Json::Value> analytics(const std::string& period);
};

}  // namespace blog::services
