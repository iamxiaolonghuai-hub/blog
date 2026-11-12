#include "repositories/UserRepository.h"

#include <drogon/drogon.h>

namespace blog::repositories {

drogon::Task<std::optional<UserRecord>> UserRepository::findByUsername(
    const std::string& username) {
    auto db = drogon::app().getDbClient();
    auto rows = co_await db->execSqlCoro(
        "SELECT id,username,password_hash,nickname FROM users "
        "WHERE username = ?",
        username);
    if (rows.empty()) co_return std::nullopt;

    const auto& r = rows[0];
    UserRecord u;
    u.id = r["id"].as<int>();
    u.username = r["username"].as<std::string>();
    u.passwordHash = r["password_hash"].as<std::string>();
    const auto& nick = r["nickname"];
    if (!nick.isNull()) u.nickname = nick.as<std::string>();
    co_return u;
}

}  // namespace blog::repositories
