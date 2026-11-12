# 龙淮的札记 · 个人博客

基于 **Vue 3 + TypeScript** 和 **C++ Drogon** 的前后端分离博客，支持文章发布、读者评论、资源分享和后台管理，使用 Docker 与 Nginx 部署。

## 在线体验

- [博客首页](http://124.222.142.28/)
- [文章列表](http://124.222.142.28/posts)
- [资源分享](http://124.222.142.28/resources)

后台和测试账号密码不随仓库公开。

## 功能

| 模块 | 功能 |
| --- | --- |
| 文章 | Markdown 编辑与预览、置顶/热门/展示权重、搜索与筛选、标签与归档、图片与附件上传、MD/Markdown/TXT/单篇 JSON 导入、MD 导出和完整备份 |
| 评论 | 读者注册与登录、楼中楼回复、图片上传、后台审核与回复 |
| 资源 | 图片和介绍文本、多个网盘链接、复制链接或提取码、后台发布与隐藏 |
| 站点 | 个人简介与头像、方向分组与关于页内容模块编辑、友情链接管理、仪表盘日/周/月/年访问趋势 |
| 音乐 | 管理员上传 MP3/OGG/WAV/M4A、选择及开关首页背景音乐、为文章指定背景音乐；前台顺序播放或单曲循环 |
| 部署 | Docker 多阶段构建、MySQL 持久化、Nginx 反向代理、首次设置管理员、数据库迁移、完整备份与恢复 |

图片上传不再使用原先的 2–5MB 应用限制；单次请求仍受前后端网关 256MB 上限约束。音频文件上限为 50MB。

## 技术栈

- 前端：Vue 3、TypeScript、Vite、Vue Router、Axios、Markdown-it、Highlight.js
- 后端：C++20、Drogon、JWT、bcrypt
- 数据与部署：MySQL 8、Docker Compose、Nginx

后端按 `controllers → services → repositories → models` 组织。管理员接口由 JWT 过滤器保护；读者评论使用独立账号，提交后需审核才会在前台显示。

## 项目结构

```text
src/                 Vue 页面、组件和 API 封装
server/src/          Drogon 控制器、服务、数据访问与模型
db/schema.sql        新库初始化表结构与示例数据
db/migrations/       已有数据库的增量迁移
deploy/              生产 Compose、Nginx 与部署说明
docs/                接口与部署文档
Dockerfile           前后端多阶段构建
docker-compose.yml   本地 MySQL 开发容器
```

## 本地运行

推荐在 **WSL/Linux** 下编译后端。需要 Node.js 20+、Docker、CMake、支持 C++20 的编译器，以及启用 MySQL 支持的 [Drogon](https://github.com/drogonframework/drogon)。

```bash
# 在项目根目录准备本地配置并启动 MySQL（首次启动执行 db/schema.sql）
cp .env.example .env.local
# 编辑 .env.local 中的本地数据库密码
docker compose --env-file .env.local up -d

# 创建本地后端配置，填入与 .env.local 相同的数据库密码
cp server/config.example.json server/config.json

# 构建后端
cmake -S server -B server/build -DCMAKE_BUILD_TYPE=Debug
cmake --build server/build -j2

# 在 server 目录运行后端
cd server
export BLOG_JWT_SECRET="$(openssl rand -hex 32)"
./build/blog_server
```

另开终端，在项目根目录启动前端：

```bash
npm ci
npm run dev
```

打开 `http://localhost:5173`。Vite 将 `/api` 和 `/uploads` 代理到 `http://localhost:8081`。新库默认没有管理员账号；本地可通过 `deploy/init-admin.sh` 创建首个管理员，脚本需要 Docker 镜像 `blog:latest`，运行前准备 `deploy/.env`。旧库升级时按顺序执行 `db/migrations/` 中尚未执行的 SQL；全新数据库只需运行 `db/schema.sql`。

## 构建与部署

```bash
npm run build
cmake --build server/build -j2
docker build -t blog:latest .
```

生产环境通过 `deploy/docker-compose.prod.yml` 启动后端与 Nginx，数据库连接和 JWT 密钥由 `deploy/.env` 注入；上传文件保存在 Docker 卷中。升级现有数据库前先备份，再执行相应迁移。详见[部署指南](docs/部署指南.md)和[接口文档](docs/API.md)。

首次部署可运行 `sudo bash deploy/setup.sh`。从 GitHub 克隆源码后，如果没有预先打包的镜像，脚本会自动构建；随后在空管理员表中要求设置用户名和密码，使用 bcrypt 保存哈希。已有管理员时不会覆盖账号。手动部署完成后运行 `sudo bash deploy/init-admin.sh`。非交互部署可临时传入 `ADMIN_USERNAME` 和 `ADMIN_PASSWORD`，不要把密码写进仓库或 `deploy/.env`。

后台「备份与恢复」可以生成包含数据库与上传文件的 ZIP、下载到电脑、上传 ZIP 恢复；也能设置每天按北京时间自动备份。自动备份默认保存在 `blog_backups` Docker 卷。可选配置 SSH 目标服务器：将后台显示的公钥加入目标用户的 `authorized_keys` 后，备份会通过 SCP 同步过去。恢复前系统自动留一份当前数据快照。

## 安全与数据

- 不要提交 `deploy/.env`、真实密码、数据库备份、构建产物或用户上传文件。
- 生产环境设置独立的 `BLOG_JWT_SECRET`；首次部署时设置自己的管理员账号和密码。
- 资源链接要求 `https://`；附件仅允许 PDF、TXT、MD、CSV、ZIP。

## 许可

本仓库暂未指定开源许可证。代码可供阅读与参考；复用请先联系作者。
