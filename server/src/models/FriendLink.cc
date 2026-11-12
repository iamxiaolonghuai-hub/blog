#include "models/FriendLink.h"

namespace blog::models {

Json::Value FriendLink::toJson() const {
    Json::Value v;
    v["id"] = id;
    v["name"] = name;
    v["url"] = url;
    v["description"] = description.empty() ? Json::Value(Json::nullValue)
                                           : Json::Value(description);
    v["logo"] = logo.empty() ? Json::Value(Json::nullValue)
                             : Json::Value(logo);
    v["sortOrder"] = sortOrder;
    v["visible"] = visible;
    return v;
}

}  // namespace blog::models
