#include "services/CommentService.h"

#include "repositories/CommentRepository.h"
#include "repositories/PostRepository.h"

namespace blog::services {

drogon::Task<std::vector<models::Comment>> CommentService::list(
    const std::optional<std::string>& status) {
    repositories::CommentRepository repo;
    co_return co_await repo.list(status);
}

drogon::Task<std::optional<std::vector<models::Comment>>>
CommentService::listBySlug(const std::string& slug) {
    repositories::PostRepository postRepo;
    const auto post = co_await postRepo.findBySlug(slug);
    if (!post) co_return std::nullopt;

    repositories::CommentRepository commentRepo;
    co_return co_await commentRepo.listByPost(post->id, true);
}

drogon::Task<std::optional<models::Comment>> CommentService::submit(
    const std::string& slug, const std::string& author,
    const std::string& email, const std::string& website,
    const std::string& content, const std::optional<int>& parentId) {
    repositories::PostRepository postRepo;
    const auto post = co_await postRepo.findBySlug(slug);
    if (!post) co_return std::nullopt;

    // 校验回复目标：必须存在，且属于同一篇文章，防止跨文章串楼
    if (parentId.has_value()) {
        repositories::CommentRepository commentRepo;
        const auto parent = co_await commentRepo.findById(*parentId);
        if (!parent || parent->postId != post->id || parent->status != "approved") {
            co_return std::nullopt;
        }
    }

    models::Comment c;
    c.postId = post->id;
    c.postTitle = post->title;
    c.parentId = parentId;
    c.author = author;
    c.email = email;
    c.website = website;
    c.content = content;
    c.status = "pending";  // 访客评论默认待审

    repositories::CommentRepository commentRepo;
    co_return co_await commentRepo.insert(c);
}

drogon::Task<std::optional<models::Comment>> CommentService::setStatus(
    int id, const std::string& status) {
    repositories::CommentRepository repo;
    co_return co_await repo.updateStatus(id, status);
}

drogon::Task<std::optional<models::Comment>> CommentService::reply(
    int id, const std::string& content) {
    repositories::CommentRepository repo;
    auto parent = co_await repo.findById(id);
    if (!parent) co_return std::nullopt;

    // 待审的父评论在回复时一并通过
    if (parent->status == "pending") {
        co_await repo.updateStatus(id, "approved");
    }

    models::Comment reply;
    reply.postId = parent->postId;
    reply.postTitle = parent->postTitle;
    reply.parentId = parent->id;
    reply.author = "管理员";
    reply.email = "admin";
    reply.content = content;
    reply.status = "approved";

    co_return co_await repo.insert(reply);
}

drogon::Task<bool> CommentService::remove(int id) {
    repositories::CommentRepository repo;
    co_return co_await repo.remove(id);
}

}  // namespace blog::services
