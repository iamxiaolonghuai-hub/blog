# Drogon 后端接口约定

前端默认请求 `/api` 前缀（开发环境由 Vite 代理到 `http://localhost:8081`）。
所有接口返回 `application/json`，字段统一使用 **camelCase**，时间使用 **ISO 8601**（如 `2026-10-20T09:30:00+08:00`）。

## 通用约定

- 成功：HTTP 200，直接返回业务 JSON（RESTful，不额外包裹 `code/message`）。
- 失败：返回对应 HTTP 状态码（404 / 401 / 400 / 500），响应体：

```json
{ "message": "post not found" }
```


---

## 1. 站点信息

`GET /api/site`

返回 `SiteInfo`：

```json
{
  "name": "龙淮的札记",
  "owner": "肖龙淮",
  "title": "系统、代码与思考",
  "bio": "一个系统程序员，写 C++ 后端，也写 Vue 前端。",
  "location": "武汉",
  "email": "hello@example.com",
  "github": "https://github.com/yourname",
  "social": [
    { "label": "GitHub", "url": "https://github.com/yourname" },
    { "label": "Email", "url": "mailto:hello@example.com" }
  ]
}
```

## 2. 文章列表

`GET /api/posts`

可选 query：

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| tag | string | 只返回包含该标签的文章 |

返回 `PostListItem[]`（**不含 content 字段**，减少体积）：

```json
[
  {
    "id": 1,
    "slug": "vue3-drogon-blog",
    "title": "用 Vue3 和 Drogon 搭建个人博客",
    "summary": "前端用 Vue3 + TypeScript，后端用 C++ Drogon……",
    "cover": null,
    "tags": ["vue", "c++", "全栈"],
    "category": "工程实践",
    "createdAt": "2026-10-20T09:30:00+08:00",
    "updatedAt": "2026-10-20T09:30:00+08:00",
    "readingMinutes": 3,
    "views": 128
  }
]
```

## 3. 文章详情

`GET /api/posts/{slug}`

返回完整 `Post`（比列表项多一个 `content`，值为 **Markdown 原文**）：

```json
{
  "id": 1,
  "slug": "vue3-drogon-blog",
  "title": "用 Vue3 和 Drogon 搭建个人博客",
  "summary": "……",
  "content": "## 整体架构\n\n这是一个典型的前后端分离结构……",
  "tags": ["vue", "c++", "全栈"],
  "category": "工程实践",
  "createdAt": "2026-10-20T09:30:00+08:00",
  "updatedAt": "2026-10-20T09:30:00+08:00",
  "readingMinutes": 3,
  "views": 128
}
```

文章不存在时返回 `404`。

## 4. 标签列表

`GET /api/tags`

返回 `TagInfo[]`，按计数倒序：

```json
[
  { "name": "c++", "count": 2 },
  { "name": "vue", "count": 1 },
  { "name": "typescript", "count": 1 }
]
```

> 前端目前可直接从文章列表派生标签，该接口为可选；后端提供更佳。

---

# 后台管理接口

除登录接口外，所有 `/api/admin/*` 接口都需要在请求头携带 JWT：

```
Authorization: Bearer <token>
```

令牌无效或过期时返回 `401`，前端会自动清除登录态并跳转登录页。

## 5. 登录

`POST /api/auth/login`

请求体：

```json
{ "username": "<首次部署设置的管理员用户名>", "password": "<首次部署设置的密码>" }
```

成功返回 `LoginResponse`：

```json
{ "token": "<JWT>", "username": "<管理员用户名>" }
```

用户名或密码错误返回 `401`：

```json
{ "message": "用户名或密码错误" }
```

## 6. 仪表盘统计

`GET /api/admin/stats`

返回 `AdminStats`：

```json
{
  "postCount": 4,
  "commentCount": 4,
  "pendingCommentCount": 2,
  "linkCount": 2,
  "totalViews": 730
}
```

> `postCount` / `linkCount` 只统计已发布 / 可见的数量。

## 7. 文章管理

### 文章列表（含草稿）

`GET /api/admin/posts`

返回完整 `Post[]`（**含 content、含草稿**），按 `updatedAt` 倒序。

### 新建文章

`POST /api/admin/posts`

请求体为 `PostInput`（不含 id / 时间戳 / 阅读时长 / 浏览量，这些由后端生成）：

```json
{
  "slug": "my-new-post",
  "title": "新文章",
  "summary": "摘要",
  "content": "## 标题\n\n正文 Markdown",
  "tags": ["vue"],
  "category": "前端",
  "status": "published"
}
```

`status` 取值 `published`（发布）或 `draft`（存草稿）。返回新建后的完整 `Post`。

### 更新文章

`PUT /api/admin/posts/{id}`

请求体同 `PostInput`，返回更新后的 `Post`。文章不存在返回 `404`。

### 删除文章

`DELETE /api/admin/posts/{id}`

成功返回 `200`（响应体可为空），文章不存在返回 `404`。

## 8. 评论管理

### 评论列表

`GET /api/admin/comments`

可选 query：

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| status | string | `pending` / `approved` / `rejected`，不传则返回全部 |

返回 `Comment[]`，按 `createdAt` 倒序。

### 审核评论（通过 / 拒绝）

`POST /api/admin/comments/{id}/status`

```json
{ "status": "approved" }
```

`status` 取值 `approved` 或 `rejected`，返回更新后的 `Comment`。

### 回复评论

`POST /api/admin/comments/{id}/reply`

```json
{ "content": "回复内容" }
```

后端创建一条 `author` 为管理员、`parentId` 为该评论的新评论（自动通过）；若原评论为待审，会一并通过。返回新评论。

### 删除评论

`DELETE /api/admin/comments/{id}`

成功返回 `200`，评论不存在返回 `404`。

## 9. 友情链接管理

### 链接列表

`GET /api/admin/links`

返回 `FriendLink[]`，按 `sortOrder` 升序。

### 新建链接

`POST /api/admin/links`

请求体为 `FriendLinkInput`（不含 id）：

```json
{
  "name": "Drogon 官网",
  "url": "https://drogon.org",
  "description": "高性能 C++ HTTP 应用框架",
  "sortOrder": 1,
  "visible": true
}
```

返回新建后的 `FriendLink`。

### 更新链接

`PUT /api/admin/links/{id}`

请求体同 `FriendLinkInput`（常用于切换 `visible` 可见性），返回更新后的 `FriendLink`。

### 删除链接

`DELETE /api/admin/links/{id}`

成功返回 `200`，不存在返回 `404`。

---

## 字段与前端类型对照

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | int | 主键 |
| slug | string | URL 唯一标识，仅含小写字母/数字/连字符 |
| title | string | 标题 |
| summary | string | 摘要 |
| content | string | Markdown 原文（仅详情接口返回） |
| tags | string[] | 标签数组 |
| category | string? | 分类，可空 |
| createdAt | string | 创建时间 ISO 8601 |
| updatedAt | string | 更新时间 ISO 8601 |
| readingMinutes | int | 预计阅读分钟数 |
| views | int? | 浏览次数，可空 |

---

## Drogon 参考实现

### 开启 CORS（前后端分离开发时）

```cpp
// main.cc
app().registerPreRoutingAdvice(
    [](const HttpRequestPtr& req,
       AdviceCallback&&,
       AdviceChainCallback&&) {
        // 简单做法：用 Drogon 内置 CORS
    });
app().addCors(
    "http://localhost:5173",
    {.methods_ = "GET, POST, OPTIONS",
     .headers_ = "Content-Type, Authorization"});
```

### 注册路由

```cpp
app().registerHandler("/api/posts",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& cb) {
        auto tag = req->getParameter("tag");
        auto posts = PostRepository::instance().list(tag);
        Json::Value arr(Json::arrayValue);
        for (const auto& p : posts) arr.append(p.toListItemJson());
        auto resp = HttpResponse::newHttpJsonResponse(arr);
        cb(resp);
    },
    {Get});

app().registerHandler("/api/posts/{slug}",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& cb) {
        auto slug = req->getParameter("slug");
        auto post = PostRepository::instance().findBySlug(slug);
        if (!post) {
            auto resp = HttpResponse::newHttpJsonResponse(
                Json::Value{{"message", "post not found"}});
            resp->setStatusCode(k404NotFound);
            cb(resp);
            return;
        }
        cb(HttpResponse::newHttpJsonResponse(post->toFullJson()));
    },
    {Get});
```

### 生产环境托管前端

把 `npm run build` 生成的 `dist/` 交给 Drogon：

```cpp
app().setDocumentRoot("./dist");
```

并将前端路由（history 模式）回退到 `index.html`，避免刷新 404：

```cpp
app().registerHandler("/{x}",
    [](const HttpRequestPtr&,
       std::function<void(const HttpResponsePtr&)>&& cb) {
        auto resp = HttpResponse::newFileResponse("./dist/index.html");
        cb(resp);
    },
    {Get});
```

> 注意该回退路由要放在 `/api` 路由之后，避免吞掉接口。

---

## 已补齐的接口（此前文档未收录）

以下接口均已实现，前端已在调用：

### 前台评论

- `GET /api/posts/{slug}/comments`：某篇文章**已通过审核**的评论。
  返回 `Comment[]`，**不含评论者 email**（隐私保护）。
- `POST /api/posts/{slug}/comments`：访客提交评论，默认进入 `pending` 待审，
  返回 `201`。请求体 `{ author, email?, website?, content, parentId? }`；
  `parentId` 必须属于同一篇文章，否则 404。
- `POST /api/upload/comment`：评论图片上传（无需登录），
  multipart 字段 `file`，返回 `{ url }`。

### 友情链接

- `GET /api/links`：前台可见的友链（`visible = 1`），按 `sortOrder` 升序。

### 上传（需 JWT）

- `POST /api/admin/upload/cover`：文章封面。
- `POST /api/admin/upload/logo`：友链 logo。
  均为 multipart 字段 `file`，支持 jpg/jpeg/png/gif/webp/avif（**不支持 svg**，
  防 XSS），限制 5MB，返回 `{ url }`（形如 `/uploads/covers/xxx.png`）。

### 后台单篇文章

- `GET /api/admin/posts/{id}`（需 JWT）：获取单篇（含正文），
  编辑页使用；列表接口 `GET /api/admin/posts` 不再返回正文。

### 仪表盘

- `GET /api/admin/stats`（需 JWT）：聚合 SQL 统计，
  返回 `{ postCount, commentCount, pendingCommentCount, linkCount, totalViews }`。

---

## 后续可扩展

- `GET /api/posts?page=&pageSize=`：分页（前端 `Paginated<T>` 类型已备好）。
- 站内搜索（`q` 参数 + FULLTEXT 索引）。
- 评论邮件通知、回复提醒。
- 评论盖楼渲染（后端已返回 `parentId`，前台目前平铺）。
