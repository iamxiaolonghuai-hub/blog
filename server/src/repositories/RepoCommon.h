#pragma once

#include <drogon/orm/Row.h>

#include <string>

namespace blog::repositories {

// MySQL DATETIME "2026-10-20 09:30:00" -> ISO 8601 "2026-10-20T09:30:00+08:00"
inline std::string toIso(const std::string& dbTime) {
    if (dbTime.size() < 19) return dbTime;
    return dbTime.substr(0, 10) + "T" + dbTime.substr(11, 8) + "+08:00";
}

// 安全读取可能为 NULL 的字符串列
inline std::string optString(const drogon::orm::Row& row, const char* col) {
    try {
        const auto& v = row[col];
        if (v.isNull()) return std::string();
        return v.as<std::string>();
    } catch (...) {
        return std::string();
    }
}

// 安全读取可能不存在的数值列（如查询里按需附加的别名）
inline long long optLong(const drogon::orm::Row& row, const char* col) {
    try {
        const auto& v = row[col];
        if (v.isNull()) return -1;
        return v.as<long long>();
    } catch (...) {
        return -1;
    }
}

}  // namespace blog::repositories
