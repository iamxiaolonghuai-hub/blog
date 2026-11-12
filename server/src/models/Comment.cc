#include "models/Comment.h"

namespace blog::models {

Json::Value Comment::toJson() const {
    Json::Value v;
    v["id"] = id;
    v["postId"] = postId;
    v["postTitle"] = postTitle;

    if (parentId.has_value()) {
        v["parentId"] = *parentId;
    } else {
        v["parentId"] = Json::nullValue;
    }

    v["author"] = author;
    v["email"] = email;
    v["website"] = website.empty() ? Json::Value(Json::nullValue)
                                   : Json::Value(website);
    v["content"] = content;
    v["status"] = status;
    v["createdAt"] = createdAt;
    return v;
}

Json::Value Comment::toPublicJson() const {
    Json::Value v;
    v["id"] = id;
    v["postId"] = postId;

    if (parentId.has_value()) {
        v["parentId"] = *parentId;
    } else {
        v["parentId"] = Json::nullValue;
    }

    v["author"] = author;
    // email 仅管理端可见，前台不输出
    v["website"] = website.empty() ? Json::Value(Json::nullValue)
                                   : Json::Value(website);
    v["content"] = content;
    v["status"] = status;
    v["createdAt"] = createdAt;
    return v;
}

}  // namespace blog::models
