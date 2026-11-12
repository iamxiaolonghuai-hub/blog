#include "services/LinkService.h"

#include "repositories/LinkRepository.h"

namespace blog::services {

drogon::Task<std::vector<models::FriendLink>> LinkService::list() {
    repositories::LinkRepository repo;
    co_return co_await repo.list();
}

drogon::Task<models::FriendLink> LinkService::create(
    const models::FriendLink& l) {
    repositories::LinkRepository repo;
    co_return co_await repo.insert(l);
}

drogon::Task<std::optional<models::FriendLink>> LinkService::update(
    int id, const models::FriendLink& l) {
    repositories::LinkRepository repo;
    co_return co_await repo.update(id, l);
}

drogon::Task<bool> LinkService::remove(int id) {
    repositories::LinkRepository repo;
    co_return co_await repo.remove(id);
}

}  // namespace blog::services
