把博客的前端和后端都用自己熟悉的技术栈实现，是理解整套 Web 链路最直接的方式。这篇文章记录我用 **Vue 3 + TypeScript** 做前端、**C++ Drogon** 做接口的完整思路。

## 整体架构

这是一个典型的前后端分离结构：

- 前端负责页面渲染、Markdown 展示、路由与交互；
- 后端只提供 JSON 接口，不关心页面长什么样；
- 文章正文以 Markdown 原文存储，前端拿到后在浏览器里渲染。

```
浏览器 (Vue3 SPA)
   │  HTTP / JSON
   ▼
Drogon (C++ 异步框架)
   │
   ▼
数据库 / Markdown 文件
```

把 Markdown 渲染放在前端而不是后端，有两个好处：后端接口返回的数据更小，同一份内容未来还能给 RSS、小程序等其他端复用。

## 前端的关键依赖

```bash
npm install vue-router axios markdown-it highlight.js
```

- `vue-router` 管理文章列表、详情、归档等路由；
- `markdown-it` 把 Markdown 转成 HTML；
- `highlight.js` 负责代码块语法高亮；
- `axios` 统一封装接口请求。

## 后端接口设计

Drogon 用起来非常轻量，一个返回文章列表的接口只需要几行：

```cpp
app().registerHandler(
    "/api/posts",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& cb) {
        auto resp = HttpResponse::newHttpJsonResponse(
            PostRepository::instance().listAll());
        cb(resp);
    },
    {Get});
```

约定好的核心接口只有四个：

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/api/posts` | 文章列表（不含正文） |
| GET | `/api/posts/{slug}` | 文章详情（含 Markdown） |
| GET | `/api/tags` | 标签及计数 |
| GET | `/api/site` | 站点信息 |

## 开发环境跨域

开发时前端跑在 Vite 的 5173 端口，后端在 8080，直接请求会有跨域问题。最省事的做法是让 Vite 代理：

```ts
server: {
  proxy: {
    '/api': { target: 'http://localhost:8080', changeOrigin: true }
  }
}
```

这样前端写 `/api/posts`，Vite 会自动转发到 Drogon，生产环境再由 Nginx 或 Drogon 统一托管静态文件即可。

## 小结

这个组合的乐趣在于：前端工程化的体验很现代，后端则是 C++ 特有的高性能和掌控感。下一步可以加上后台管理和 JWT 登录，让写文章也在自己的系统里完成。
