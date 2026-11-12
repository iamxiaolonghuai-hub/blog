# 博客后端（C++ Drogon）

面向对象分层：`controllers`（HTTP）→ `services`（业务）→ `repositories`（数据）→ `models`（实体），鉴权用 `filters`，通用能力放 `utils`。

## 环境要求

- 支持 C++20 的编译器（g++ ≥ 12）
- CMake ≥ 3.16
- [Drogon](https://github.com/drogonframework/drogon)（需启用 MySQL 支持）
- MySQL 8.0 / MariaDB

### 安装 Drogon（Arch Linux）

```bash
sudo pacman -S --needed c-ares util-linux zlib sqlite git mariadb-libs cmake
git clone https://github.com/drogonframework/drogon.git
cd drogon
git submodule update --init
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_ORM=ON -DBUILD_MYSQL=ON
make -j$(nproc)
sudo make install
```

## 初始化数据库

```bash
mysql -u root -p < ../db/schema.sql
```

> 脚本会创建 `blog` 库和全部表，不预置管理员。首次部署时运行 `deploy/init-admin.sh` 设置账号和密码。

## 配置

编辑 `config.json`：

1. **数据库连接**（`db_clients`）：填写 `host / port / user / passwd`。
2. **JWT 密钥**：优先读取环境变量 `BLOG_JWT_SECRET`；未设置时回退到
   `custom_config.jwt_secret`。两者都为空时服务会**拒绝启动**（fail-fast）。
   推荐用环境变量注入，避免密钥出现在配置文件里：
   ```bash
   openssl rand -hex 32
   export BLOG_JWT_SECRET=<生成的随机串>
   ```
3. **站点信息**（`custom_config.site`）：名称、简介、GitHub 等。
4. **端口**：本地开发统一使用 **8081**（`config.json` 监听、`.env.local`
   代理一致）；生产可改回 8080，注意与前端 `.env` 同步。

> 注意：`config.json` 里的 `cors` 段不是 Drogon 文档化配置项，**不会生效**。
> 开发环境由 Vite 代理（同源）规避跨域；生产建议前后端同域部署（见下文
> Nginx 配置），因此无需 CORS。

## 编译

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## 运行

```bash
# 在 server 目录下运行（需要能读到 config.json）
./build/blog_server
```

## 目录结构

```
server/
├── CMakeLists.txt
├── config.json
└── src/
    ├── main.cc
    ├── controllers/      # 路由与请求处理
    │   ├── SiteController.h
    │   ├── PostController.h
    │   ├── TagController.h
    │   ├── PublicLinkController.h
    │   ├── AuthController.h
    │   ├── AdminPostController.h
    │   ├── AdminCommentController.h
    │   ├── AdminLinkController.h
    │   └── AdminStatsController.h
    ├── services/         # 业务逻辑
    ├── repositories/     # SQL 数据访问
    ├── models/           # 实体与 JSON 序列化
    ├── filters/          # JWT 鉴权过滤器
    └── utils/            # JWT、bcrypt、统一响应
```

## 部署到云服务器

> 推荐：本地 WSL Arch 用 Release 编译，上传二进制（需服务器同为 x86_64 Linux、并装好 Drogon 运行库）；
> 更稳妥的是在服务器上直接编译。下面以服务器编译 + Nginx + systemd 为例。

### 1. 服务器准备（Ubuntu/Debian 为例）

```bash
# 编译依赖（Arch 用 pacman 装对应包）
sudo apt update
sudo apt install -y g++ cmake git libjsoncpp-dev uuid-dev zlib1g-dev \
  libssl-dev libmysqlclient-dev libc-ares-dev
# 源码编译安装 Drogon（步骤同上文「安装 Drogon」）
```

### 2. 上传、初始化数据库、编译

```bash
# 上传整个项目后
mysql -u root -p < db/schema.sql
cd server
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 3. 生产配置

- `config.json` 监听端口按需调整（本地开发统一 8081），与前端部署方式保持一致。
- `db_clients` 填生产 MySQL 地址与账号。
- 通过环境变量 `BLOG_JWT_SECRET` 注入随机密钥（`openssl rand -hex 32`）。
- 前后端同域部署（Nginx 反代 `/api/` 与 `/uploads/`），无需 CORS。

### 4. systemd 守护

创建 `/etc/systemd/system/blog.service`：

```ini
[Unit]
Description=Blog Server (Drogon)
After=network.target mysql.service

[Service]
Type=simple
WorkingDirectory=/opt/blog/server
ExecStart=/opt/blog/server/build/blog_server
Restart=on-failure
RestartSec=3
User=www-data

[Install]
WantedBy=multi-user.target
```

启用：

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now blog
sudo systemctl status blog
```

### 5. 前端构建

```bash
# 构建前把 .env.local 里 VITE_PROXY_TARGET 去掉依赖（生产用同域 /api）
# .env.production 中设置 VITE_API_BASE=/api、VITE_USE_MOCK=false
npm run build      # 产物在 dist/
```

### 6. Nginx 配置（托管前端 + 反代后端）

```nginx
server {
    listen 80;
    server_name example.com;
    root /var/www/blog/dist;
    index index.html;

    # 后端接口反向代理
    location /api/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # 上传的图片（封面/评论图/logo）由 Drogon 静态服务提供
    location /uploads/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
    }

    # 前端单页路由（history 模式回退）
    location / {
        try_files $uri $uri/ /index.html;
    }
}
```

```bash
sudo nginx -t && sudo systemctl reload nginx
```

> 有 HTTPS 需求时用 certbot 申请证书，或在 Nginx 配置 443。

### 修改管理员密码

生成 bcrypt 哈希：

```bash
htpasswd -bnBC 10 "" "你的新密码" | tr -d ':\n'
```

更新数据库：

```sql
UPDATE users SET password_hash = '<新哈希>' WHERE username = 'admin';
```

## 接口一览

见 `../docs/API.md`，前台与后台接口均已约定。
