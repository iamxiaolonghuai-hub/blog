#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <vector>

#include "models/FriendLink.h"

namespace blog::services {

class LinkService {
public:
    drogon::Task<std::vector<models::FriendLink>> list();
    drogon::Task<models::FriendLink> create(const models::FriendLink& l);
    drogon::Task<std::optional<models::FriendLink>> update(
        int id, const models::FriendLink& l);
    drogon::Task<bool> remove(int id);
};

}  // namespace blog::services
