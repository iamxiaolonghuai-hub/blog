#!/bin/sh
# 博客后端容器入口：
# 1. 用环境变量替换 config 模板中的占位符，生成 /app/config.json
# 2. 把前端构建产物复制到共享卷（供 Nginx 托管）
# 3. 启动 blog_server（JWT 密钥由 BLOG_JWT_SECRET 环境变量提供，见 ServiceCommon.h）
set -e

: "${DB_HOST:=127.0.0.1}"
: "${DB_PORT:=3306}"
: "${DB_NAME:=blog}"
: "${DB_USER:=root}"
: "${DB_PASSWORD:?请设置 DB_PASSWORD}"

sed -e "s|@DB_HOST@|${DB_HOST}|g" \
    -e "s|@DB_PORT@|${DB_PORT}|g" \
    -e "s|@DB_NAME@|${DB_NAME}|g" \
    -e "s|@DB_USER@|${DB_USER}|g" \
    -e "s|@DB_PASSWORD@|${DB_PASSWORD}|g" \
    /app/config.template.json > /app/config.json

# 前端静态文件 → 共享卷（nginx 只读挂载同一个卷）
if [ -d /var/www/dist ] && [ -d /app/www/dist ]; then
    cp -r /app/www/dist/. /var/www/dist/
fi

mkdir -p /app/backups
python3 /app/backup_tool.py worker >> /app/backups/worker.log 2>&1 &

exec /app/blog_server
