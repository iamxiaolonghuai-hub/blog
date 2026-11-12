#include "services/PostService.h"

#include "repositories/PostRepository.h"

namespace blog::services {

drogon::Task<std::vector<models::Post>> PostService::listPublished(
    const std::string& tag) {
    repositories::PostRepository repo;
    co_return co_await repo.listPublished(tag);
}

drogon::Task<std::optional<models::Post>> PostService::getBySlug(
    const std::string& slug) {
    repositories::PostRepository repo;
    auto post = co_await repo.findBySlug(slug);
    if (post) {
        co_await repo.incrementViews(post->id);
        post->views += 1;
    }
    co_return post;
}

drogon::Task<std::vector<std::pair<std::string, int>>>
PostService::tagCounts() {
    repositories::PostRepository repo;
    co_return co_await repo.tagCounts();
}

drogon::Task<std::vector<models::Post>> PostService::listAll() {
    repositories::PostRepository repo;
    co_return co_await repo.listAll();
}

drogon::Task<std::optional<models::Post>> PostService::getById(int id) {
    repositories::PostRepository repo;
    co_return co_await repo.findById(id);
}

drogon::Task<models::Post> PostService::create(const models::Post& p) {
    repositories::PostRepository repo;
    co_return co_await repo.insert(p, p.tags);
}

drogon::Task<std::optional<models::Post>> PostService::update(
    int id, const models::Post& p) {
    repositories::PostRepository repo;
    co_return co_await repo.update(id, p, p.tags);
}

drogon::Task<bool> PostService::remove(int id) {
    repositories::PostRepository repo;
    co_return co_await repo.remove(id);
}

}  // namespace blog::services
