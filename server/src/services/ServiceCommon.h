#pragma once

#include <drogon/drogon.h>

#include <cstdlib>
#include <stdexcept>
#include <string>

#include "utils/Jwt.h"

namespace blog::services {

// 读取 JWT 密钥：优先环境变量 BLOG_JWT_SECRET，其次 config.json 的
// custom_config.jwt_secret；两者都为空时抛出异常（fail-fast），
// 避免静默使用空密钥签名。
inline std::string jwtSecret() {
    const char* envSecret = std::getenv("BLOG_JWT_SECRET");
    std::string secret = envSecret != nullptr ? envSecret : "";
    if (secret.empty()) {
        const auto& cfg = drogon::app().getCustomConfig();
        if (cfg.isMember("jwt_secret") && cfg["jwt_secret"].isString()) {
            secret = cfg["jwt_secret"].asString();
        }
    }
    if (secret.empty()) {
        throw std::runtime_error(
            "JWT secret is not configured: set the BLOG_JWT_SECRET "
            "environment variable or custom_config.jwt_secret");
    }
    return secret;
}

// 从配置读取密钥并构造 JWT
inline blog::utils::Jwt makeJwt() {
    return blog::utils::Jwt(jwtSecret());
}

}  // namespace blog::services
