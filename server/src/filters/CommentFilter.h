#pragma once
#include <drogon/drogon.h>

namespace blog::filters {
class CommentFilter : public drogon::HttpFilter<CommentFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};
}
