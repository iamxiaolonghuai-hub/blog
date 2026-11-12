#pragma once

#include <drogon/orm/DbClient.h>

#include <optional>
#include <string>

namespace blog::services {

struct LoginResult {
    std::string token;
    std::string username;
};

class AuthService {
public:
    // 校验账号密码，成功返回 token
    drogon::Task<std::optional<LoginResult>> login(
        const std::string& username, const std::string& password);
};

}  // namespace blog::services
