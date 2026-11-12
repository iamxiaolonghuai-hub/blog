#pragma once

#include <json/json.h>

#include <optional>
#include <string>

namespace blog::utils {

// Base64URL（无填充）编解码
std::string base64UrlEncode(const std::string& input);
std::optional<std::string> base64UrlDecode(const std::string& input);

// JWT（HS256）签发与校验
class Jwt {
public:
    explicit Jwt(std::string secret);

    // 生成 token，默认有效期 24 小时
    std::string generate(int userId, const std::string& username,
                         int ttlSeconds = 86400,
                         const std::string& role = "admin") const;

    // 校验签名与有效期，成功返回 payload
    std::optional<Json::Value> verify(const std::string& token) const;

private:
    std::string secret_;
};

}  // namespace blog::utils
