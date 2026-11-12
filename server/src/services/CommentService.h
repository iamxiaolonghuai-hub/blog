#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <string>
#include <vector>

#include "models/Comment.h"

namespace blog::services {

class CommentService {
public:
    drogon::Task<std::vector<models::Comment>> list(
        const std::optional<std::string>& status = std::nullopt);

    // 前台：获取某篇文章（按 slug）已通过的评论
    drogon::Task<std::optional<std::vector<models::Comment>>> listBySlug(
        const std::string& slug);

    // 前台：访客提交评论（默认 pending 待审）
    // 返回新评论；文章不存在时返回 nullopt
    drogon::Task<std::optional<models::Comment>> submit(
        const std::string& slug, const std::string& author,
        const std::string& email, const std::string& website,
        const std::string& content,
        const std::optional<int>& parentId = std::nullopt);

    drogon::Task<std::optional<models::Comment>> setStatus(
        int id, const std::string& status);

    // 管理员回复：创建回复评论，若父评论待审则一并通过
    drogon::Task<std::optional<models::Comment>> reply(
        int id, const std::string& content);

    drogon::Task<bool> remove(int id);
};

}  // namespace blog::services
