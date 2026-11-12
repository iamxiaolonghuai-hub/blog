#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <string>
#include <vector>

#include "models/Comment.h"

namespace blog::repositories {

class CommentRepository {
public:
    drogon::Task<std::vector<models::Comment>> list(
        const std::optional<std::string>& status = std::nullopt);

    // 查询某篇文章下的评论；onlyApproved=true 时仅返回已通过的（前台用）
    drogon::Task<std::vector<models::Comment>> listByPost(
        int postId, bool onlyApproved);

    drogon::Task<std::optional<models::Comment>> findById(int id);

    drogon::Task<models::Comment> insert(const models::Comment& c);

    drogon::Task<std::optional<models::Comment>> updateStatus(
        int id, const std::string& status);

    drogon::Task<bool> remove(int id);
};

}  // namespace blog::repositories
