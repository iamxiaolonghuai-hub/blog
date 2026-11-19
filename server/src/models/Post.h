#pragma once

#include <json/json.h>

#include <string>
#include <vector>

namespace blog::models {

struct Post {
    int id = 0;
    std::string slug;
    std::string title;
    std::string summary;
    std::string content;
    std::string cover;
    std::string category;
    int musicId = 0;
    bool isPinned = false;
    bool isHot = false;
    int displayWeight = 0;
    std::string status;  // draft / published / hidden
    int views = 0;
    std::string createdAt;  // ISO 8601
    std::string updatedAt;
    std::vector<std::string> tags;

    // 正文字节数。列表查询不取正文，由 SQL 的 LENGTH(content) 附加，
    // 值 <0 表示未知（此时按 1 分钟兜底）。
    long long contentBytes = -1;

    // withContent=false 时生成列表项（不含正文）
    Json::Value toJson(bool withContent = true) const;

    // 预计阅读时长（分钟）
    int readingMinutes() const;
};

}  // namespace blog::models
