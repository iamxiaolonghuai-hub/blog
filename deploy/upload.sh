#!/usr/bin/env bash
# ============================================================
#  从本地一键上传并部署到腾讯云服务器
#  用法（Git Bash / WSL / macOS 终端）：
#    ./deploy/upload.sh root@你的公网IP
#  过程中会提示输入 SSH 密码 2 次（上传一次、部署一次）
# ============================================================
set -euo pipefail

TARGET="${1:-}"
[ -n "$TARGET" ] || { echo "用法: ./deploy/upload.sh root@你的公网IP"; exit 1; }

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# 选择镜像包（优先全量离线包）
BUNDLE="blog-bundle.tar.gz"
[ -f "$BUNDLE" ] || BUNDLE="blog-image.tar.gz"
[ -f "$BUNDLE" ] || { echo "错误：找不到镜像包，请先 docker build + docker save"; exit 1; }

[ -f db/schema.sql ]                        || { echo "错误：缺少 db/schema.sql"; exit 1; }
[ -f deploy/docker-compose.prod.yml ]       || { echo "错误：缺少 deploy/docker-compose.prod.yml"; exit 1; }

echo "==> 目标服务器：$TARGET"
echo "==> 镜像包：$BUNDLE ($(du -h "$BUNDLE" | cut -f1))"
echo

echo "==> 上传文件到 /opt/blog （大文件请耐心等待）…"
ssh "$TARGET" "mkdir -p /opt/blog"
tar cf - db deploy "$BUNDLE" | ssh "$TARGET" "tar xf - -C /opt/blog"
echo "==> 上传完成"

echo
echo "==> 执行服务器部署脚本（首次会自动安装 Docker，约 2-5 分钟）…"
ssh -t "$TARGET" "cd /opt/blog && chmod +x deploy/setup.sh && ./deploy/setup.sh"
