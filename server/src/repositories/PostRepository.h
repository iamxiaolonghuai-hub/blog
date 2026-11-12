#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "models/Post.h"

namespace blog::repositories {

class PostRepository {
public:
    // 前台：已发布列表（不含正文）
    drogon::Task<std::vector<models::Post>> listPublished(
        const std::string& tag = "");

    // 前台：按 slug 找（含正文）
    drogon::Task<std::optional<models::Post>> findBySlug(
        const std::string& slug);

    // 浏览量 +1
    drogon::Task<void> incrementViews(int id);

    // 后台：全部（含草稿、正文）
    drogon::Task<std::vector<models::Post>> listAll();

    // 后台：按 id 找
    drogon::Task<std::optional<models::Post>> findById(int id);

    drogon::Task<models::Post> insert(
        const models::Post& p, const std::vector<std::string>& tags);

    drogon::Task<std::optional<models::Post>> update(
        int id, const models::Post& p, const std::vector<std::string>& tags);

    drogon::Task<bool> remove(int id);
    drogon::Task<std::optional<models::Post>> updateDisplay(int id, bool pinned, bool hot, int weight);

    // 标签 + 计数
    drogon::Task<std::vector<std::pair<std::string, int>>> tagCounts();

private:
    drogon::Task<void> syncTags(int postId,
                                const std::vector<std::string>& tags);
    drogon::Task<void> fillTags(std::vector<models::Post>& posts);
};

}  // namespace blog::repositories
