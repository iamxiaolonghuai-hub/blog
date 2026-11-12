#include "filters/CommentFilter.h"
#include "services/ServiceCommon.h"
#include "utils/Response.h"

namespace blog::filters {
void CommentFilter::doFilter(const drogon::HttpRequestPtr& req,
                             drogon::FilterCallback&& fcb,
                             drogon::FilterChainCallback&& fccb) {
    const auto auth = req->getHeader("Authorization");
    if (auth.rfind("Bearer ", 0) != 0) {
        fcb(utils::errorResponse(drogon::k401Unauthorized, "请先登录评论账号"));
        return;
    }
    std::optional<Json::Value> payload;
    try { payload = services::makeJwt().verify(auth.substr(7)); }
    catch (const std::exception& e) {
        fcb(utils::errorResponse(drogon::k500InternalServerError, e.what()));
        return;
    }
    if (!payload || !(*payload)["username"].isString() ||
        !(*payload)["sub"].isInt() ||
        ((*payload).get("role", "admin").asString() != "reader" &&
         (*payload).get("role", "admin").asString() != "admin")) {
        fcb(utils::errorResponse(drogon::k401Unauthorized, "登录已过期，请重新登录"));
        return;
    }
    req->getAttributes()->insert("comment_username", (*payload)["username"].asString());
    fccb();
}
}
