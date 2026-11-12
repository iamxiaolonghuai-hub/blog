#include "services/UploadService.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>

namespace blog::services {

namespace {

// 允许的图片扩展名（小写）。注意：不收 svg——svg 可内嵌脚本，
// 配合直链访问会构成存储型 XSS。
bool isAllowedImage(const std::string& ext) {
    static const std::vector<std::string> kAllowed = {
        "jpg", "jpeg", "png", "gif", "webp", "avif"};
    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return std::find(kAllowed.begin(), kAllowed.end(), e) != kAllowed.end();
}
bool isAllowedAttachment(const std::string& ext) {
    static const std::vector<std::string> kAllowed = {
        "pdf", "txt", "md", "csv", "zip"};
    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return std::find(kAllowed.begin(), kAllowed.end(), e) != kAllowed.end();
}
bool isAllowedAudio(const std::string& ext) {
    static const std::vector<std::string> kAllowed = {"mp3", "ogg", "wav", "m4a"};
    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return std::find(kAllowed.begin(), kAllowed.end(), e) != kAllowed.end();
}

// 生成唯一文件名：时间戳 + 随机串
std::string uniqueName(const std::string& ext) {
    using namespace std::chrono;
    const auto ms =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 0xFFFFFF);
    char randPart[8];
    std::snprintf(randPart, sizeof(randPart), "%06x", dist(gen));

    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return std::to_string(ms) + "-" + randPart + "." + e;
}

void ensureDir(const std::string& path) {
    // 递归创建目录（0755）
    std::string cur;
    for (size_t i = 0; i < path.size(); ++i) {
        cur.push_back(path[i]);
        if (path[i] == '/' || i + 1 == path.size()) {
            if (!cur.empty()) mkdir(cur.c_str(), 0755);
        }
    }
}

}  // namespace

UploadResult UploadService::handle(drogon::HttpRequestPtr req,
                                   const std::string& subdir,
                                   const std::string& fieldName,
                                   size_t maxBytes, bool attachment, bool audio) {
    UploadResult result;

    drogon::MultiPartParser parser;
    if (parser.parse(req) != 0) {
        result.error = "请求不是有效的 multipart/form-data";
        return result;
    }

    const auto& files = parser.getFiles();
    const drogon::HttpFile* target = nullptr;
    for (const auto& f : files) {
        if (f.getItemName() == fieldName || fieldName.empty()) {
            target = &f;
            break;
        }
    }
    if (target == nullptr) {
        // 若字段名不匹配，取第一个文件兜底
        if (!files.empty()) {
            target = &files[0];
        } else {
            result.error = "没有找到上传的文件";
            return result;
        }
    }

    const std::string ext{target->getFileExtension()};
    if (audio ? !isAllowedAudio(ext) :
        (attachment ? !isAllowedAttachment(ext) : !isAllowedImage(ext))) {
        result.error = audio ? "仅支持 mp3/ogg/wav/m4a 音频" :
                       (attachment ? "仅支持 pdf/txt/md/csv/zip 文件" :
                                     "仅支持 jpg/jpeg/png/gif/webp/avif 图片");
        return result;
    }

    if (target->fileLength() > maxBytes) {
        result.error = "文件过大，超过允许大小";
        return result;
    }

    // 保存到 document_root 下的 uploads/<subdir>/
    const std::string baseDir = "./www/uploads/" + subdir;
    ensureDir(baseDir);

    const std::string fileName = uniqueName(ext);
    const std::string fullPath = baseDir + "/" + fileName;
    if (target->saveAs(fullPath) != 0) {
        result.error = "文件保存失败";
        return result;
    }

    result.ok = true;
    result.url = "/uploads/" + subdir + "/" + fileName;
    return result;
}

}  // namespace blog::services
