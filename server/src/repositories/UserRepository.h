#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <string>

namespace blog::repositories {

struct UserRecord {
    int id = 0;
    std::string username;
    std::string passwordHash;
    std::string nickname;
};

class UserRepository {
public:
    drogon::Task<std::optional<UserRecord>> findByUsername(
        const std::string& username);
};

}  // namespace blog::repositories
