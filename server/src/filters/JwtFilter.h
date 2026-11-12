#pragma once

#include <drogon/HttpFilter.h>

namespace blog::filters {

// 校验 Authorization: Bearer <JWT>，通过后把用户信息写入 request attributes
class JwtFilter : public drogon::HttpFilter<JwtFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};

}  // namespace blog::filters
