#pragma once

#include <json/json.h>

#include <string>

namespace blog::models {

struct FriendLink {
    int id = 0;
    std::string name;
    std::string url;
    std::string description;
    std::string logo;
    int sortOrder = 0;
    bool visible = true;

    Json::Value toJson() const;
};

}  // namespace blog::models
