#pragma once

#include <string>

namespace blog::utils {

// bcrypt 密码哈希与校验（基于 libxcrypt，算法 $2b$）
class Password {
public:
    static std::string hash(const std::string& plain, int cost = 10);
    static bool verify(const std::string& plain, const std::string& hash);
};

}  // namespace blog::utils
