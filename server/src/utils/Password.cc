#include "utils/Password.h"

#include <crypt.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>

namespace blog::utils {

namespace {

// bcrypt 使用的 64 字符集
const char* kBcryptChars =
    "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

std::string randomSalt(size_t length) {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    std::string salt;
    salt.reserve(length);
    unsigned char buf[16];
    size_t filled = 0;
    while (filled < length) {
        urandom.read(reinterpret_cast<char*>(buf), sizeof(buf));
        const size_t got = static_cast<size_t>(urandom.gcount());
        for (size_t i = 0; i < got && filled < length; ++i) {
            salt.push_back(kBcryptChars[buf[i] & 63]);
            ++filled;
        }
    }
    return salt;
}

}  // namespace

std::string Password::hash(const std::string& plain, int cost) {
    char setting[32];
    std::snprintf(setting, sizeof(setting), "$2b$%02d$%s", cost,
                  randomSalt(22).c_str());

    crypt_data data{};
    const char* result = crypt_r(plain.c_str(), setting, &data);
    return result != nullptr ? std::string(result) : std::string();
}

bool Password::verify(const std::string& plain, const std::string& hash) {
    crypt_data data{};
    const char* result = crypt_r(plain.c_str(), hash.c_str(), &data);
    return result != nullptr && hash == result;
}

}  // namespace blog::utils
