#pragma once

#include <json/json.h>

#include <optional>
#include <string>

namespace blog::models {

struct Comment {
    int id = 0;
    int postId = 0;
    std::string postTitle;  // join posts 得到，不入库
    std::optional<int> parentId;
    std::string author;
    std::string email;
    std::string website;
    std::string content;
    std::string status;  // pending / approved / rejected
    std::string createdAt;

    // 完整版（管理端使用，包含 email）
    Json::Value toJson() const;

    // 公开版（前台使用，剔除评论者邮箱，防止泄露）
    Json::Value toPublicJson() const;
};

}  // namespace blog::models
