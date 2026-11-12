#pragma once

#include <drogon/drogon.h>

#include <string>

namespace blog::services {

// 上传结果：成功时 url 为可访问路径，失败时 error 为错误信息
struct UploadResult {
    bool ok = false;
    std::string url;
    std::string error;
};

// 文件上传服务：把 multipart 文件保存到 www/uploads/<subdir>/，
// 通过 Drogon document_root 以 /uploads/... 访问。
class UploadService {
public:
    // subdir: 子目录（covers / comments / logos）
    // fieldName: 期望的表单字段名（一般为 "file"）
    // maxBytes: 允许的最大字节数
    UploadResult handle(drogon::HttpRequestPtr req, const std::string& subdir,
                        const std::string& fieldName = "file",
                        size_t maxBytes = 5 * 1024 * 1024,
                        bool attachment = false, bool audio = false);
};

}  // namespace blog::services
