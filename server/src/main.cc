#include <drogon/drogon.h>
#include <iostream>
#include <string>
#include "utils/Password.h"

// 前台控制器
#include "controllers/PostController.h"
#include "controllers/PublicCommentController.h"
#include "controllers/PublicLinkController.h"
#include "controllers/SiteController.h"
#include "controllers/TagController.h"
#include "controllers/VisitController.h"
#include "controllers/MusicController.h"

// 后台控制器
#include "controllers/AdminCommentController.h"
#include "controllers/AdminLinkController.h"
#include "controllers/AdminPostController.h"
#include "controllers/AdminStatsController.h"
#include "controllers/AuthController.h"
#include "controllers/UploadController.h"
#include "controllers/ReaderAuthController.h"
#include "controllers/ResourceController.h"
#include "controllers/BackupController.h"
#include "filters/CommentFilter.h"

// 过滤器
#include "filters/JwtFilter.h"

#include "services/ServiceCommon.h"

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--hash-password") {
        std::string password;
        if (!std::getline(std::cin, password) || password.size() < 8 || password.size() > 128) {
            std::cerr << "Password must contain 8-128 bytes\n";
            return 1;
        }
        const auto hash = blog::utils::Password::hash(password);
        if (hash.empty()) return 1;
        std::cout << hash << '\n';
        return 0;
    }
    drogon::app().loadConfigFile("config.json");

    // fail-fast：JWT 密钥缺失时直接拒绝启动，避免带病运行
    try {
        blog::services::jwtSecret();
    } catch (const std::exception& e) {
        LOG_FATAL << e.what();
        return 1;
    }

    drogon::app().run();
    return 0;
}
