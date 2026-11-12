#include "repositories/LinkRepository.h"

#include <drogon/drogon.h>

#include "repositories/RepoCommon.h"

using namespace blog::models;

namespace blog::repositories {

namespace {

FriendLink mapRow(const drogon::orm::Row& row) {
    FriendLink l;
    l.id = row["id"].as<int>();
    l.name = row["name"].as<std::string>();
    l.url = row["url"].as<std::string>();
    l.description = optString(row, "description");
    l.logo = optString(row, "logo");
    l.sortOrder = row["sort_order"].as<int>();
    l.visible = row["visible"].as<int>() != 0;
    return l;
}

}  // namespace

drogon::Task<std::vector<FriendLink>> LinkRepository::list() {
    auto db = drogon::app().getDbClient();
    auto rows = co_await db->execSqlCoro(
        "SELECT * FROM links ORDER BY sort_order ASC");
    std::vector<FriendLink> result;
    for (const auto& r : rows) result.push_back(mapRow(r));
    co_return result;
}

drogon::Task<std::optional<FriendLink>> LinkRepository::findById(int id) {
    auto db = drogon::app().getDbClient();
    auto rows = co_await db->execSqlCoro(
        "SELECT * FROM links WHERE id = ?", id);
    if (rows.empty()) co_return std::nullopt;
    co_return mapRow(rows[0]);
}

drogon::Task<FriendLink> LinkRepository::insert(const FriendLink& l) {
    auto db = drogon::app().getDbClient();
    auto r = co_await db->execSqlCoro(
        "INSERT INTO links(name,url,description,logo,sort_order,visible) "
        "VALUES(?,?,?,?,?,?)",
        l.name, l.url, l.description, l.logo, l.sortOrder,
        l.visible ? 1 : 0);
    const int newId = static_cast<int>(r.insertId());
    auto saved = co_await findById(newId);
    co_return saved.value();
}

drogon::Task<std::optional<FriendLink>> LinkRepository::update(
    int id, const FriendLink& l) {
    auto db = drogon::app().getDbClient();
    // 不用 affectedRows 判断存在性：值未变化时 MySQL 返回 0
    co_await db->execSqlCoro(
        "UPDATE links SET name=?,url=?,description=?,logo=?,sort_order=?,"
        "visible=? WHERE id=?",
        l.name, l.url, l.description, l.logo, l.sortOrder,
        l.visible ? 1 : 0, id);
    co_return co_await findById(id);
}

drogon::Task<bool> LinkRepository::remove(int id) {
    auto db = drogon::app().getDbClient();
    auto r = co_await db->execSqlCoro("DELETE FROM links WHERE id = ?", id);
    co_return r.affectedRows() > 0;
}

}  // namespace blog::repositories
