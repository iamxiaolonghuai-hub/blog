#include "utils/Jwt.h"

#include <openssl/crypto.h>
#include <openssl/hmac.h>

#include <ctime>
#include <sstream>
#include <vector>

namespace blog::utils {

namespace {

const char* kBase64UrlTable =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string hmacSha256(const std::string& key, const std::string& data) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         result, &len);
    return std::string(reinterpret_cast<char*>(result), len);
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

// 常量时间比较，避免时序侧信道
bool secureEquals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

std::optional<Json::Value> parseJson(const std::string& text) {
    Json::Value value;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream stream(text);
    if (!Json::parseFromStream(reader, stream, &value, &errors)) {
        return std::nullopt;
    }
    return value;
}

}  // namespace

std::string base64UrlEncode(const std::string& input) {
    std::string out;
    int val = 0;
    int valBits = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valBits += 8;
        while (valBits >= 0) {
            out.push_back(kBase64UrlTable[(val >> valBits) & 0x3F]);
            valBits -= 6;
        }
    }
    if (valBits > -6) {
        out.push_back(kBase64UrlTable[((val << 8) >> (valBits + 8)) & 0x3F]);
    }
    return out;  // 无填充
}

std::optional<std::string> base64UrlDecode(const std::string& input) {
    std::string out;
    std::vector<int> table(256, -1);
    for (int i = 0; i < 64; ++i) {
        table[static_cast<unsigned char>(kBase64UrlTable[i])] = i;
    }
    int val = 0;
    int valBits = -8;
    for (char ch : input) {
        if (ch == '=') break;
        int d = table[static_cast<unsigned char>(ch)];
        if (d == -1) return std::nullopt;
        val = (val << 6) + d;
        valBits += 6;
        if (valBits >= 0) {
            out.push_back(static_cast<char>((val >> valBits) & 0xFF));
            valBits -= 8;
        }
    }
    return out;
}

Jwt::Jwt(std::string secret) : secret_(std::move(secret)) {}

std::string Jwt::generate(int userId, const std::string& username,
                          int ttlSeconds, const std::string& role) const {
    const auto now = static_cast<long>(std::time(nullptr));

    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    Json::Value payload;
    payload["sub"] = userId;
    payload["username"] = username;
    payload["role"] = role;
    payload["iat"] = now;
    payload["exp"] = now + ttlSeconds;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const std::string h = base64UrlEncode(Json::writeString(builder, header));
    const std::string p = base64UrlEncode(Json::writeString(builder, payload));
    const std::string signingInput = h + "." + p;
    const std::string sig = base64UrlEncode(hmacSha256(secret_, signingInput));

    return signingInput + "." + sig;
}

std::optional<Json::Value> Jwt::verify(const std::string& token) const {
    const auto parts = split(token, '.');
    if (parts.size() != 3) return std::nullopt;

    // header 必须声明 HS256，防止算法混淆
    const auto header = parseJson(
        base64UrlDecode(parts[0]).value_or(std::string()));
    if (!header || !header->isMember("alg") ||
        (*header)["alg"].asString() != "HS256") {
        return std::nullopt;
    }

    const std::string signingInput = parts[0] + "." + parts[1];
    const std::string expected = base64UrlEncode(hmacSha256(secret_, signingInput));
    if (!secureEquals(expected, parts[2])) return std::nullopt;

    const auto payloadStr = base64UrlDecode(parts[1]);
    if (!payloadStr) return std::nullopt;

    const auto payload = parseJson(*payloadStr);
    if (!payload) return std::nullopt;

    // 必须携带过期时间，缺失视为无效 token
    if (!payload->isMember("exp") || !(*payload)["exp"].isNumeric()) {
        return std::nullopt;
    }
    if ((*payload)["exp"].asInt64() <=
        static_cast<long long>(std::time(nullptr))) {
        return std::nullopt;  // 已过期
    }
    return payload;
}

}  // namespace blog::utils
