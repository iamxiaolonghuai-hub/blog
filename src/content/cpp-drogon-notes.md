Drogon 是一个基于 C++17 的异步 HTTP 框架，性能在各类 Web 框架基准测试中常年靠前。它的核心卖点是：**异步非阻塞 + 协程支持 + 自带 ORM**。

## 为什么是 C++

常见的后端语言已经足够好用，但 C++ 在几个场景下仍然有优势：

- 单机需要极高并发，对延迟和内存占用敏感；
- 服务端要复用已有的 C/C++ 算法或库；
- 单纯想理解一个高性能网络框架内部是怎么转的。

## 安装与编译

```bash
git clone https://github.com/drogonframework/drogon
cd drogon && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) && sudo make install
```

依赖里最重要的是 `trantor`（Drogon 的网络库）、`jsoncpp` 和 `uuid`。

## 协程写法

Drogon 支持 C++20 协程后，异步代码终于不再被回调拆碎：

```cpp
Task<> fetchPost(const std::string& slug) {
    auto rows = co_await db->execSqlCoro(
        "SELECT * FROM posts WHERE slug = $1", slug);
    if (rows.empty()) {
        co_return;
    }
    const auto& row = rows.front();
    std::string title = row["title"].as<std::string>();
    LOG_INFO << "got post: " << title;
}
```

`co_await` 让数据库调用看起来像同步代码，但底层依然是事件循环驱动，不会阻塞线程。

## 路由的两种方式

```cpp
// 方式一：lambda 注册
app().registerHandler("/api/health",
    [](const HttpRequestPtr&, auto&& cb) {
        auto resp = HttpResponse::newHttpJsonResponse(
            Json::Value{{"status", "ok"}});
        cb(resp);
    });

// 方式二：HttpController 类 + 路径映射
// 适合接口较多、需要按模块组织的项目
```

接口少的时候用 `registerHandler` 最快；像博客后端这种有文章、标签、用户多个模块的，用 `HttpController` 按文件拆分更清晰。

## 静态文件托管

```cpp
app().setDocumentRoot("./frontend_dist");
```

部署时把前端 `npm run build` 产物交给 Drogon 直接托管，就不再需要额外的 Nginx，整个服务是一个二进制加一个静态目录，非常干净。

> 对性能有要求的场景，记得编译时打开 Release，并合理设置连接线程数（通常等于 CPU 核数）。

Drogon 的文档和示例相当完善，把官方的 `example` 跑一遍，基本就能上手自己的项目了。
