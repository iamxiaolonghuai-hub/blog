# syntax=docker/dockerfile:1

# =====================================================================
# 多阶段构建：前端 + 后端（含 Drogon 源码编译）+ 运行镜像
# 本地：docker build -t blog:latest .
# 导出：docker save blog:latest | gzip > blog-image.tar.gz
# =====================================================================

# ---------------------------------------------------------------------
# 1. 前端构建（Vue3 + TS + Vite）
# ---------------------------------------------------------------------
FROM node:20-alpine AS frontend-builder
WORKDIR /web
COPY package.json package-lock.json ./
RUN npm ci
COPY . .
RUN npm run build          # 产物在 /web/dist

# ---------------------------------------------------------------------
# 2. 后端构建（Ubuntu 22.04，源码编译 Drogon 1.9.14）
# ---------------------------------------------------------------------
FROM ubuntu:22.04 AS backend-builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake make git ca-certificates \
        libssl-dev libjsoncpp-dev uuid-dev zlib1g-dev \
        libmariadb-dev libmariadb-dev-compat libc-ares-dev \
    && rm -rf /var/lib/apt/lists/*

# 编译安装 Drogon（与本地开发版本一致）
RUN git clone --depth 1 --shallow-submodules --recurse-submodules -j8 \
        --branch v1.9.14 \
        https://github.com/drogonframework/drogon.git /tmp/drogon \
    && cmake -S /tmp/drogon -B /tmp/drogon/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF -DBUILD_TESTING=OFF \
        -DBUILD_BROTLI=OFF -DBUILD_YAML_CONFIG=OFF \
        -DBUILD_ORM=ON -DBUILD_MYSQL=ON \
        -DBUILD_SQLITE3=OFF -DBUILD_POSTGRESQL=OFF -DBUILD_REDIS=OFF \
    && cmake --build /tmp/drogon/build -j"$(nproc)" \
    && cmake --install /tmp/drogon/build \
    && ldconfig

# 编译博客后端
COPY server /src
RUN cmake -S /src -B /src/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /src/build -j"$(nproc)"

# ---------------------------------------------------------------------
# 3. 运行镜像（只带二进制 + 运行库，体积小）
# ---------------------------------------------------------------------
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3 libjsoncpp25 zlib1g libmariadb3 \
        libc-ares2 libuuid1 python3 tzdata mariadb-client openssh-client \
    && rm -rf /var/lib/apt/lists/*

# Drogon / trantor 的动态库
COPY --from=backend-builder /usr/local/lib/libdrogon.so* /usr/local/lib/
COPY --from=backend-builder /usr/local/lib/libtrantor.so* /usr/local/lib/
RUN ldconfig

WORKDIR /app
COPY --from=backend-builder /src/build/blog_server ./blog_server
COPY server/config.docker.json ./config.template.json
COPY server/docker-entrypoint.sh ./docker-entrypoint.sh
COPY server/backup_tool.py ./backup_tool.py
RUN chmod +x ./docker-entrypoint.sh

# 前端产物（启动时复制到共享卷，交给 Nginx 托管）
COPY --from=frontend-builder /web/dist ./www/dist
RUN mkdir -p ./www/uploads

ENV APP_PORT=8081
EXPOSE 8081
ENTRYPOINT ["./docker-entrypoint.sh"]
