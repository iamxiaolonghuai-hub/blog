#include "repositories/CommentRepository.h"

#include <drogon/drogon.h>

#include "repositories/RepoCommon.h"

using namespace blog::models;

namespace blog::repositories {

namespace {

Comment mapRow(const drogon::orm::Row& row) {
    Comment c;
    c.id = row["id"].as<int>();
    c.postId = row["post_id"].as<int>();
    c.postTitle = row["post_title"].as<std::string>();

    const auto& parent = row["parent_id"];
    if (!parent.isNull()) c.parentId = parent.as<int>();

    c.author = row["author"].as<std::string>();
    c.email = row["email"].as<std::string>();
    c.website = optString(row, "website");
    c.content = row["content"].as<std::string>();
    c.status = row["status"].as<std::string>();
    c.createdAt = toIso(row["created_at"].as<std::string>());
    return c;
}

const char* kBaseSql =
    "SELECT c.*, p.title AS post_title FROM comments c "
    "JOIN posts p ON p.id = c.post_id ";

}  // namespace

drogon::Task<std::vector<Comment>> CommentRepository::list(
    const std::optional<std::string>& status) {
    auto db = drogon::app().getDbClient();
    std::vector<Comment> result;
    if (status.has_value()) {
        auto rows = co_await db->execSqlCoro(
            std::string(kBaseSql) +
                "WHERE c.status = ? ORDER BY c.created_at DESC",
            *status);
        for (const auto& r : rows) result.push_back(mapRow(r));
    } else {
        auto rows = co_await db->execSqlCoro(
            std::string(kBaseSql) + "ORDER BY c.created_at DESC");
        for (const auto& r : rows) result.push_back(mapRow(r));
    }
    co_return result;
}

drogon::Task<std::vector<Comment>> CommentRepository::listByPost(
    int postId, bool onlyApproved) {
    auto db = drogon::app().getDbClient();
    std::vector<Comment> result;
    if (onlyApproved) {
        auto rows = co_await db->execSqlCoro(
            std::string(kBaseSql) +
                "WHERE c.post_id = ? AND c.status = 'approved' "
                "ORDER BY c.created_at ASC",
            postId);
        for (const auto& r : rows) result.push_back(mapRow(r));
    } else {
        auto rows = co_await db->execSqlCoro(
            std::string(kBaseSql) +
                "WHERE c.post_id = ? ORDER BY c.created_at ASC",
            postId);
        for (const auto& r : rows) result.push_back(mapRow(r));
    }
    co_return result;
}

drogon::Task<std::optional<Comment>> CommentRepository::findById(int id) {
    auto db = drogon::app().getDbClient();
    auto rows = co_await db->execSqlCoro(
        std::string(kBaseSql) + "WHERE c.id = ?", id);
    if (rows.empty()) co_return std::nullopt;
    co_return mapRow(rows[0]);
}

drogon::Task<Comment> CommentRepository::insert(const Comment& c) {
    auto db = drogon::app().getDbClient();
    int newId = 0;
    if (c.parentId.has_value()) {
        auto r = co_await db->execSqlCoro(
            "INSERT INTO comments(post_id,parent_id,author,email,website,"
            "content,status) VALUES(?,?,?,?,?,?,?)",
            c.postId, c.parentId.value(), c.author, c.email, c.website,
            c.content, c.status);
        newId = static_cast<int>(r.insertId());
    } else {
        auto r = co_await db->execSqlCoro(
            "INSERT INTO comments(post_id,parent_id,author,email,website,"
            "content,status) VALUES(?,?,?,?,?,?,?)",
            c.postId, nullptr, c.author, c.email, c.website, c.content,
            c.status);
        newId = static_cast<int>(r.insertId());
    }
    auto saved = co_await findById(newId);
    co_return saved.value();
}

drogon::Task<std::optional<Comment>> CommentRepository::updateStatus(
    int id, const std::string& status) {
    auto db = drogon::app().getDbClient();
    // 不用 affectedRows 判断存在性：状态未变化时 MySQL 返回 0
    co_await db->execSqlCoro(
        "UPDATE comments SET status = ? WHERE id = ?", status, id);
    co_return co_await findById(id);
}

drogon::Task<bool> CommentRepository::remove(int id) {
    auto db = drogon::app().getDbClient();
    auto r = co_await db->execSqlCoro("DELETE FROM comments WHERE id = ?", id);
    co_return r.affectedRows() > 0;
}

}  // namespace blog::repositories
