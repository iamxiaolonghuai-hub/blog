#include "models/Post.h"

#include <cmath>

namespace blog::models {

namespace {

Json::Value nullableString(const std::string& value) {
    return value.empty() ? Json::Value(Json::nullValue) : Json::Value(value);
}

}  // namespace

int Post::readingMinutes() const {
    // 详情页用真实正文，列表页退回 SQL 附加的字节数
    const long long bytes =
        content.empty() && contentBytes >= 0 ? contentBytes
                                             : static_cast<long long>(content.size());
    return std::max(1, static_cast<int>(std::round(
                           static_cast<double>(bytes) / 400.0)));
}

Json::Value Post::toJson(bool withContent) const {
    Json::Value v;
    v["id"] = id;
    v["slug"] = slug;
    v["title"] = title;
    v["summary"] = summary;
    if (withContent) v["content"] = content;
    v["cover"] = nullableString(cover);

    Json::Value tagArray(Json::arrayValue);
    for (const auto& t : tags) tagArray.append(t);
    v["tags"] = tagArray;

    v["category"] = nullableString(category);
    v["musicId"] = musicId > 0 ? Json::Value(musicId) : Json::Value(Json::nullValue);
    v["isPinned"] = isPinned;
    v["isHot"] = isHot;
    v["displayWeight"] = displayWeight;
    v["status"] = status;
    v["createdAt"] = createdAt;
    v["updatedAt"] = updatedAt;
    v["readingMinutes"] = readingMinutes();
    v["views"] = views;
    return v;
}

}  // namespace blog::models
