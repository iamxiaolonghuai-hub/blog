#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>

#include <string>

namespace blog::utils {

// 构造 JSON 响应
inline drogon::HttpResponsePtr jsonResponse(
    const Json::Value& data,
    drogon::HttpStatusCode code = drogon::k200OK) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(data);
    resp->setStatusCode(code);
    return resp;
}

// 构造错误响应 { "message": "..." }
inline drogon::HttpResponsePtr errorResponse(
    drogon::HttpStatusCode code, const std::string& message) {
    Json::Value body;
    body["message"] = message;
    return jsonResponse(body, code);
}

}  // namespace blog::utils
