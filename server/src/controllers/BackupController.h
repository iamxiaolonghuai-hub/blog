#pragma once
#include <drogon/drogon.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <thread>
#include "utils/Response.h"

namespace blog::controllers {
class BackupController : public drogon::HttpController<BackupController> {
    static constexpr const char* root = "./backups";
    static Json::Value read(const std::string& file) {
        std::ifstream input(std::string(root) + "/" + file);
        Json::Value value;
        if (input) input >> value;
        return value;
    }
    static bool write(const std::string& file, const Json::Value& value) {
        std::filesystem::create_directories(root);
        const std::string temp = std::string(root) + "/" + file + ".tmp";
        std::ofstream output(temp, std::ios::trunc);
        if (!output) return false;
        output << value;
        output.close();
        std::filesystem::rename(temp, std::string(root) + "/" + file);
        return true;
    }
    static void launch(const std::string& action) {
        std::thread([action] {
            const std::string command = "/usr/bin/python3 /app/backup_tool.py " + action +
                " >> /app/backups/worker.log 2>&1";
            std::system(command.c_str());
        }).detach();
    }
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(BackupController::list, "/api/admin/backups", drogon::Get, "blog::filters::JwtFilter");
    ADD_METHOD_TO(BackupController::settings, "/api/admin/backups/settings", drogon::Put, "blog::filters::JwtFilter");
    ADD_METHOD_TO(BackupController::run, "/api/admin/backups/run", drogon::Post, "blog::filters::JwtFilter");
    ADD_METHOD_TO(BackupController::download, "/api/admin/backups/{name}", drogon::Get, "blog::filters::JwtFilter");
    ADD_METHOD_TO(BackupController::restore, "/api/admin/backups/restore", drogon::Post, "blog::filters::JwtFilter");
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr) {
        std::filesystem::create_directories(root);
        Json::Value body;
        body["settings"] = read("settings.json");
        body["status"] = read("status.json");
        Json::Value files(Json::arrayValue);
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            const auto name = entry.path().filename().string();
            if (entry.is_regular_file() && std::regex_match(name, std::regex("(blog|before-restore)-[0-9]{8}-[0-9]{6}\\.zip"))) {
                Json::Value file;
                file["name"] = name;
                file["bytes"] = static_cast<Json::UInt64>(entry.file_size());
                files.append(file);
            }
        }
        body["files"] = files;
        std::ifstream key(std::string(root) + "/remote_key.pub");
        std::string line;
        std::getline(key, line);
        body["remotePublicKey"] = line;
        co_return utils::jsonResponse(body);
    }
    drogon::Task<drogon::HttpResponsePtr> settings(drogon::HttpRequestPtr req) {
        const auto input = req->getJsonObject();
        if (!input || !(*input)["enabled"].isBool() || !(*input)["remoteEnabled"].isBool() ||
            !(*input)["time"].isString() || !(*input)["retentionDays"].isInt())
            co_return utils::errorResponse(drogon::k400BadRequest, "备份设置格式错误");
        const auto time = (*input)["time"].asString();
        if (!std::regex_match(time, std::regex("(0[0-9]|1[0-9]|2[0-3]):[0-5][0-9]")) ||
            (*input)["retentionDays"].asInt() < 1 || (*input)["retentionDays"].asInt() > 365)
            co_return utils::errorResponse(drogon::k400BadRequest, "时间或保留天数无效");
        Json::Value value;
        value["enabled"] = (*input)["enabled"];
        value["time"] = time;
        value["retentionDays"] = (*input)["retentionDays"];
        value["remoteEnabled"] = (*input)["remoteEnabled"];
        for (const auto& key : {"remoteHost", "remoteUser", "remotePath"})
            value[key] = (*input).get(key, "").asString();
        value["remotePort"] = (*input).get("remotePort", 22).asInt();
        if (value["remotePort"].asInt() < 1 || value["remotePort"].asInt() > 65535 ||
            value["remoteHost"].asString().size() > 255 ||
            value["remoteUser"].asString().size() > 80 ||
            value["remotePath"].asString().size() > 500)
            co_return utils::errorResponse(drogon::k400BadRequest, "远程服务器设置无效");
        const auto old = read("settings.json");
        value["lastScheduledDate"] = old.get("lastScheduledDate", "");
        if (!write("settings.json", value))
            co_return utils::errorResponse(drogon::k500InternalServerError, "设置保存失败");
        co_return utils::jsonResponse(value);
    }
    drogon::Task<drogon::HttpResponsePtr> run(drogon::HttpRequestPtr) {
        launch("manual");
        Json::Value body;
        body["message"] = "备份已启动";
        co_return utils::jsonResponse(body, drogon::k202Accepted);
    }
    drogon::Task<drogon::HttpResponsePtr> download(drogon::HttpRequestPtr, std::string name) {
        if (!std::regex_match(name, std::regex("(blog|before-restore)-[0-9]{8}-[0-9]{6}\\.zip")))
            co_return utils::errorResponse(drogon::k400BadRequest, "文件名无效");
        const auto path = std::string(root) + "/" + name;
        if (!std::filesystem::is_regular_file(path))
            co_return utils::errorResponse(drogon::k404NotFound, "备份不存在");
        auto response = drogon::HttpResponse::newFileResponse(path);
        response->addHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
        response->addHeader("Cache-Control", "no-store");
        co_return response;
    }
    drogon::Task<drogon::HttpResponsePtr> restore(drogon::HttpRequestPtr req) {
        drogon::MultiPartParser parser;
        if (parser.parse(req) != 0 || parser.getFiles().empty())
            co_return utils::errorResponse(drogon::k400BadRequest, "请选择完整备份 ZIP");
        const auto& file = parser.getFiles()[0];
        if (file.fileLength() > 200 * 1024 * 1024 || file.getFileExtension() != "zip")
            co_return utils::errorResponse(drogon::k400BadRequest, "只接受不超过 200MB 的 ZIP");
        std::filesystem::create_directories(root);
        const auto path = std::string(root) + "/incoming.zip";
        if (file.saveAs(path) != 0)
            co_return utils::errorResponse(drogon::k500InternalServerError, "上传失败");
        launch("restore /app/backups/incoming.zip");
        Json::Value body;
        body["message"] = "恢复已启动，恢复前会自动备份当前数据";
        co_return utils::jsonResponse(body, drogon::k202Accepted);
    }
};
}
