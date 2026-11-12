#include "services/StatsService.h"

#include <drogon/drogon.h>
#include <map>

#include "repositories/CommentRepository.h"
#include "repositories/LinkRepository.h"
#include "repositories/PostRepository.h"

namespace blog::services {

Json::Value AdminStats::toJson() const {
    Json::Value v;
    v["postCount"] = postCount;
    v["commentCount"] = commentCount;
    v["pendingCommentCount"] = pendingCommentCount;
    v["linkCount"] = linkCount;
    v["totalViews"] = totalViews;
    v["siteViews"] = siteViews;
    v["guestVisitors"] = guestVisitors;
    v["registeredVisitors"] = registeredVisitors;
    v["readerAccounts"] = readerAccounts;
    return v;
}

drogon::Task<AdminStats> StatsService::stats() {
    // 直接用聚合 SQL，避免把全表（含正文）拉进内存计数
    auto db = drogon::app().getDbClient();
    AdminStats s;

    auto postRows = co_await db->execSqlCoro(
        "SELECT "
        "COUNT(CASE WHEN status = 'published' THEN 1 END) AS published, "
        "COALESCE(SUM(views), 0) AS total_views FROM posts");
    if (!postRows.empty()) {
        s.postCount = postRows[0]["published"].as<int>();
        s.totalViews = postRows[0]["total_views"].as<int>();
    }

    auto commentRows = co_await db->execSqlCoro(
        "SELECT COUNT(*) AS total, "
        "COUNT(CASE WHEN status = 'pending' THEN 1 END) AS pending "
        "FROM comments");
    if (!commentRows.empty()) {
        s.commentCount = commentRows[0]["total"].as<int>();
        s.pendingCommentCount = commentRows[0]["pending"].as<int>();
    }

    auto linkRows = co_await db->execSqlCoro(
        "SELECT COUNT(*) AS visible FROM links WHERE visible = 1");
    if (!linkRows.empty()) {
        s.linkCount = linkRows[0]["visible"].as<int>();
    }

    auto visitRows = co_await db->execSqlCoro(
        "SELECT COUNT(*) AS views, "
        "COUNT(DISTINCT CASE WHEN reader_id IS NULL THEN visitor_key END) AS guests, "
        "COUNT(DISTINCT reader_id) AS readers FROM visit_events");
    if (!visitRows.empty()) {
        s.siteViews = visitRows[0]["views"].as<int>();
        s.guestVisitors = visitRows[0]["guests"].as<int>();
        s.registeredVisitors = visitRows[0]["readers"].as<int>();
    }
    auto readerRows = co_await db->execSqlCoro("SELECT COUNT(*) AS total FROM reader_accounts");
    if (!readerRows.empty()) s.readerAccounts = readerRows[0]["total"].as<int>();

    co_return s;
}

drogon::Task<Json::Value> StatsService::analytics(const std::string& period) {
    std::string format;
    std::string window;
    if (period == "day") { format = "%Y-%m-%d %H"; window = "INTERVAL 24 HOUR"; }
    else if (period == "week") { format = "%Y-%m-%d"; window = "INTERVAL 7 DAY"; }
    else if (period == "month") { format = "%Y-%m-%d"; window = "INTERVAL 30 DAY"; }
    else { format = "%Y-%m"; window = "INTERVAL 12 MONTH"; }

    const auto sql =
        "SELECT DATE_FORMAT(DATE_ADD(created_at, INTERVAL 8 HOUR), ?) AS bucket, "
        "COUNT(*) AS views, "
        "COUNT(DISTINCT CASE WHEN reader_id IS NULL THEN visitor_key END) AS guests, "
        "COUNT(DISTINCT reader_id) AS readers "
        "FROM visit_events WHERE created_at >= UTC_TIMESTAMP() - " + window +
        " GROUP BY bucket ORDER BY bucket";
    const auto rows = co_await drogon::app().getDbClient()->execSqlCoro(sql, format);
    std::map<std::string, Json::Value> grouped;
    for (const auto& row : rows) {
        Json::Value point;
        const auto bucket = row["bucket"].as<std::string>();
        point["bucket"] = bucket;
        point["views"] = row["views"].as<int>();
        point["guests"] = row["guests"].as<int>();
        point["readers"] = row["readers"].as<int>();
        point["newAccounts"] = 0;
        grouped[bucket] = point;
    }
    const auto registrationSql =
        "SELECT DATE_FORMAT(created_at, ?) AS bucket, COUNT(*) AS registrations "
        "FROM reader_accounts WHERE created_at >= NOW() - " + window +
        " GROUP BY bucket ORDER BY bucket";
    const auto registrationRows = co_await drogon::app().getDbClient()->execSqlCoro(registrationSql, format);
    for (const auto& row : registrationRows) {
        const auto bucket = row["bucket"].as<std::string>();
        auto& point = grouped[bucket];
        if (!point.isMember("bucket")) {
            point["bucket"] = bucket;
            point["views"] = 0;
            point["guests"] = 0;
            point["readers"] = 0;
        }
        point["newAccounts"] = row["registrations"].as<int>();
    }
    Json::Value points(Json::arrayValue);
    for (const auto& entry : grouped) points.append(entry.second);
    Json::Value result;
    result["period"] = period;
    result["points"] = points;
    co_return result;
}

}  // namespace blog::services
