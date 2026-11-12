#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <vector>

#include "models/FriendLink.h"

namespace blog::repositories {

class LinkRepository {
public:
    drogon::Task<std::vector<models::FriendLink>> list();

    drogon::Task<std::optional<models::FriendLink>> findById(int id);

    drogon::Task<models::FriendLink> insert(const models::FriendLink& l);

    drogon::Task<std::optional<models::FriendLink>> update(
        int id, const models::FriendLink& l);

    drogon::Task<bool> remove(int id);
};

}  // namespace blog::repositories
