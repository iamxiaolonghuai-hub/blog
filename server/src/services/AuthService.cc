#include "services/AuthService.h"

#include "repositories/UserRepository.h"
#include "services/ServiceCommon.h"
#include "utils/Password.h"

namespace blog::services {

drogon::Task<std::optional<LoginResult>> AuthService::login(
    const std::string& username, const std::string& password) {
    repositories::UserRepository userRepo;
    auto user = co_await userRepo.findByUsername(username);
    if (!user) co_return std::nullopt;

    if (!utils::Password::verify(password, user->passwordHash)) {
        co_return std::nullopt;
    }

    auto jwt = makeJwt();
    const std::string token = jwt.generate(user->id, user->username);
    co_return LoginResult{token, user->username};
}

}  // namespace blog::services
