#include "repositories/PostRepository.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <cctype>
#include <map>

#include "repositories/RepoCommon.h"

using namespace blog::models;

namespace blog::repositories {

namespace {

// 简单的标签 slug 生成（小写、非字母数字转连字符）
std::string tagSlug(const std::string& name) {
    std::string s;
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            s.push_back(static_cast<char>(std::tolower(c)));
        } else if (!s.empty() && s.back() != '-') {
            s.push_back('-');
        }
    }
    if (!s.empty() && s.back() == '-') s.pop_back();
    return s.empty() ? name : s;
}

Post mapRow(const drogon::orm::Row& row, bool withContent) {
    Post p;
    p.id = row["id"].as<int>();
    p.slug = row["slug"].as<std::string>();
    p.title = row["title"].as<std::string>();
    p.summary = optString(row, "summary");
    if (withContent) {
        p.content = row["content"].as<std::string>();
    } else {
        p.contentBytes = optLong(row, "content_bytes");
    }
    p.cover = optString(row, "cover");
    p.category = optString(row, "category");
    if (!row["music_id"].isNull()) p.musicId = row["music_id"].as<int>();
    p.isPinned = row["is_pinned"].as<int>() != 0;
    p.isHot = row["is_hot"].as<int>() != 0;
    p.displayWeight = row["display_weight"].as<int>();
    p.status = row["status"].as<std::string>();
    p.views = row["views"].as<int>();
    p.createdAt = toIso(row["created_at"].as<std::string>());
    p.updatedAt = toIso(row["updated_at"].as<std::string>());
    return p;
}

}  // namespace

drogon::Task<void> PostRepository::fillTags(std::vector<Post>& posts) {
    if (posts.empty()) co_return;
    auto db = drogon::app().getDbClient();

    // 只查本批文章的标签，避免全表扫描（id 为 int，to_string 拼接安全）
    std::string in;
    for (size_t i = 0; i < posts.size(); ++i) {
        if (i > 0) in.push_back(',');
        in += std::to_string(posts[i].id);
    }

    auto rows = co_await db->execSqlCoro(
        "SELECT pt.post_id, t.name FROM post_tags pt "
        "JOIN tags t ON t.id = pt.tag_id "
        "WHERE pt.post_id IN (" + in + ")");

    std::map<int, std::vector<std::string>> grouped;
    for (const auto& r : rows) {
        grouped[r["post_id"].as<int>()].push_back(r["name"].as<std::string>());
    }
    for (auto& p : posts) {
        auto it = grouped.find(p.id);
        if (it != grouped.end()) p.tags = it->second;
    }
}

drogon::Task<void> PostRepository::syncTags(
    int postId, const std::vector<std::string>& tags) {
    auto db = drogon::app().getDbClient();
    co_await db->execSqlCoro("DELETE FROM post_tags WHERE post_id = ?", postId);
    for (const auto& name : tags) {
        if (name.empty()) continue;
        co_await db->execSqlCoro(
            "INSERT IGNORE INTO tags(name, slug) VALUES(?, ?)", name,
            tagSlug(name));
        auto r = co_await db->execSqlCoro(
            "SELECT id FROM tags WHERE name = ?", name);
        if (r.size() > 0) {
            co_await db->execSqlCoro(
                "INSERT IGNORE INTO post_tags(post_id, tag_id) VALUES(?, ?)",
                postId, r[0]["id"].as<int>());
        }
    }
}

drogon::Task<std::vector<Post>> PostRepository::listPublished(
    const std::string& tag) {
    auto db = drogon::app().getDbClient();
    std::vector<Post> result;
    if (tag.empty()) {
        auto rows = co_await db->execSqlCoro(
            "SELECT *, LENGTH(content) AS content_bytes "
            "FROM posts WHERE status = 'published' "
            "ORDER BY is_pinned DESC, display_weight DESC, is_hot DESC, published_at DESC, id DESC");
        for (const auto& r : rows) result.push_back(mapRow(r, false));
    } else {
        auto rows = co_await db->execSqlCoro(
            "SELECT p.*, LENGTH(p.content) AS content_bytes FROM posts p "
            "JOIN post_tags pt ON pt.post_id = p.id "
            "JOIN tags t ON t.id = pt.tag_id "
            "WHERE p.status = 'published' AND t.name = ? "
            "ORDER BY p.is_pinned DESC, p.display_weight DESC, p.is_hot DESC, p.published_at DESC, p.id DESC",
            tag);
        for (const auto& r : rows) result.push_back(mapRow(r, false));
    }
    co_await fillTags(result);
    co_return result;
}

drogon::Task<std::optional<Post>> PostRepository::findBySlug(
    const std::string& slug) {
    auto db = drogon::app().getDbClient();
    auto rows = co_await db->execSqlCoro(
        "SELECT * FROM posts WHERE slug = ? AND status = 'published'", slug);
    if (rows.empty()) co_return std::nullopt;
    std::vector<Post> list{mapRow(rows[0], true)};
    co_await fillTags(list);
    co_return list.front();
}

drogon::Task<void> PostRepository::incrementViews(int id) {
    auto db = drogon::app().getDbClient();
    co_await db->execSqlCoro(
        "UPDATE posts SET views = views + 1 WHERE id = ?", id);
    co_return;
}

drogon::Task<std::vector<Post>> PostRepository::listAll() {
    auto db = drogon::app().getDbClient();
    // 后台列表不需要正文，仅附带字节数供阅读时长估算
    auto rows = co_await db->execSqlCoro(
        "SELECT *, LENGTH(content) AS content_bytes FROM posts "
        "ORDER BY updated_at DESC");
    std::vector<Post> result;
    for (const auto& r : rows) result.push_back(mapRow(r, false));
    co_await fillTags(result);
    co_return result;
}

drogon::Task<std::optional<Post>> PostRepository::findById(int id) {
    auto db = drogon::app().getDbClient();
    auto rows = co_await db->execSqlCoro("SELECT * FROM posts WHERE id = ?", id);
    if (rows.empty()) co_return std::nullopt;
    std::vector<Post> list{mapRow(rows[0], true)};
    co_await fillTags(list);
    co_return list.front();
}

drogon::Task<Post> PostRepository::insert(
    const Post& p, const std::vector<std::string>& tags) {
    auto db = drogon::app().getDbClient();
    int newId = 0;
    if (p.status == "published") {
        auto r = co_await db->execSqlCoro(
            "INSERT INTO posts(slug,title,summary,content,cover,category,music_id,is_pinned,is_hot,display_weight,"
            "status,published_at) VALUES(?,?,?,?,?,?,NULLIF(?,0),?,?,?,?,NOW())",
            p.slug, p.title, p.summary, p.content, p.cover, p.category,
            p.musicId, p.isPinned, p.isHot, p.displayWeight, p.status);
        newId = static_cast<int>(r.insertId());
    } else {
        auto r = co_await db->execSqlCoro(
            "INSERT INTO posts(slug,title,summary,content,cover,category,music_id,is_pinned,is_hot,display_weight,"
            "status,published_at) VALUES(?,?,?,?,?,?,NULLIF(?,0),?,?,?,?,NULL)",
            p.slug, p.title, p.summary, p.content, p.cover, p.category,
            p.musicId, p.isPinned, p.isHot, p.displayWeight, p.status);
        newId = static_cast<int>(r.insertId());
    }
    co_await syncTags(newId, tags);
    auto saved = co_await findById(newId);
    co_return saved.value();
}

drogon::Task<std::optional<Post>> PostRepository::update(
    int id, const Post& p, const std::vector<std::string>& tags) {
    auto db = drogon::app().getDbClient();
    // 注意不能用 affectedRows 判断记录是否存在：
    // MySQL 对"值没有变化"的 UPDATE 返回 0，会误判成 404。
    co_await db->execSqlCoro(
        "UPDATE posts SET slug=?,title=?,summary=?,content=?,cover=?,"
        "category=?,music_id=NULLIF(?,0),is_pinned=?,is_hot=?,display_weight=?,status=?,published_at=COALESCE(published_at,"
        "IF(?='published',NOW(),NULL)) WHERE id=?",
        p.slug, p.title, p.summary, p.content, p.cover, p.category, p.musicId,
        p.isPinned, p.isHot, p.displayWeight, p.status,
        p.status, id);
    co_await syncTags(id, tags);
    co_return co_await findById(id);
}

drogon::Task<bool> PostRepository::remove(int id) {
    auto db = drogon::app().getDbClient();
    auto r = co_await db->execSqlCoro("DELETE FROM posts WHERE id = ?", id);
    co_return r.affectedRows() > 0;
}

drogon::Task<std::optional<Post>> PostRepository::updateDisplay(int id, bool pinned, bool hot, int weight) {
    auto db = drogon::app().getDbClient();
    co_await db->execSqlCoro(
        "UPDATE posts SET is_pinned=?, is_hot=?, display_weight=? WHERE id=?",
        pinned, hot, weight, id);
    co_return co_await findById(id);
}

drogon::Task<std::vector<std::pair<std::string, int>>>
PostRepository::tagCounts() {
    auto db = drogon::app().getDbClient();
    auto rows = co_await db->execSqlCoro(
        "SELECT t.name, COUNT(pt.post_id) AS cnt FROM tags t "
        "JOIN post_tags pt ON pt.tag_id = t.id "
        "JOIN posts p ON p.id = pt.post_id AND p.status = 'published' "
        "GROUP BY t.id ORDER BY cnt DESC");
    std::vector<std::pair<std::string, int>> result;
    for (const auto& r : rows) {
        result.emplace_back(r["name"].as<std::string>(), r["cnt"].as<int>());
    }
    co_return result;
}

}  // namespace blog::repositories
