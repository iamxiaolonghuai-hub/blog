#include "filters/JwtFilter.h"

#include <drogon/drogon.h>

#include "services/ServiceCommon.h"
#include "utils/Jwt.h"
#include "utils/Response.h"

using namespace blog::utils;

namespace blog::filters {

void JwtFilter::doFilter(const drogon::HttpRequestPtr& req,
                         drogon::FilterCallback&& fcb,
                         drogon::FilterChainCallback&& fccb) {
    const std::string auth = req->getHeader("Authorization");
    std::string token;
    if (auth.rfind("Bearer ", 0) == 0) {
        token = auth.substr(7);
    }

    std::optional<Json::Value> payload;
    try {
        blog::utils::Jwt jwt(blog::services::jwtSecret());
        payload = jwt.verify(token);
    } catch (const std::exception& e) {
        // 密钥未配置属于服务端错误，而非 401
        fcb(errorResponse(drogon::k500InternalServerError, e.what()));
        return;
    }

    if (!payload) {
        fcb(errorResponse(drogon::k401Unauthorized, "未授权或登录已过期"));
        return;
    }
    if ((*payload).get("role", "admin").asString() != "admin") {
        fcb(errorResponse(drogon::k403Forbidden, "需要管理员权限"));
        return;
    }

    req->getAttributes()->insert("user_id", (*payload)["sub"].asInt());
    req->getAttributes()->insert("username",
                                 (*payload)["username"].asString());
    fccb();
}

}  // namespace blog::filters
