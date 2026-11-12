#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "models/Post.h"

namespace blog::services {

class PostService {
public:
    // 前台
    drogon::Task<std::vector<models::Post>> listPublished(
        const std::string& tag = "");
    drogon::Task<std::optional<models::Post>> getBySlug(
        const std::string& slug);
    drogon::Task<std::vector<std::pair<std::string, int>>> tagCounts();

    // 后台
    drogon::Task<std::vector<models::Post>> listAll();
    drogon::Task<std::optional<models::Post>> getById(int id);
    drogon::Task<models::Post> create(const models::Post& p);
    drogon::Task<std::optional<models::Post>> update(
        int id, const models::Post& p);
    drogon::Task<bool> remove(int id);
};

}  // namespace blog::services
