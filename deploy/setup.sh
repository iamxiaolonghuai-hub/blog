#!/usr/bin/env bash
# ============================================================
#  博客上云一键部署脚本
#  适用：腾讯云 Ubuntu / Debian / CentOS / Rocky / OpenCloudOS / TencentOS
#  用法（root 用户）：
#    cd /opt/blog && chmod +x deploy/setup.sh && ./deploy/setup.sh
#
#  支持三种数据库部署方式，首次运行会让你选：
#    1) 独立共享 MySQL 容器  —— 推荐，多项目共用，更新博客不影响数据库
#    2) compose 自带 MySQL   —— 单项目自包含
#    3) 腾讯云托管 TencentDB —— 已买实例，脚本自动导入表结构
# ============================================================
set -euo pipefail

# ---------- 颜色输出 ----------
c_reset=$'\033[0m'; c_green=$'\033[32m'; c_yellow=$'\033[33m'; c_red=$'\033[31m'; c_blue=$'\033[36m'
info()  { printf "%s[INFO]%s %s\n"  "$c_blue"   "$c_reset" "$*"; }
ok()    { printf "%s[ OK ]%s %s\n"  "$c_green"  "$c_reset" "$*"; }
warn()  { printf "%s[WARN]%s %s\n"  "$c_yellow" "$c_reset" "$*"; }
fail()  { printf "%s[FAIL]%s %s\n"  "$c_red"    "$c_reset" "$*"; exit 1; }

# ---------- 前置检查 ----------
[ "$(id -u)" = "0" ] || fail "请用 root 运行：sudo su - 之后再执行本脚本"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
info "项目根目录：$ROOT_DIR"

[ -f deploy/docker-compose.prod.yml ] || fail "缺少 deploy/docker-compose.prod.yml，请确认目录结构"
[ -f db/schema.sql ]                  || fail "缺少 db/schema.sql，请连同 db 目录一起上传"
ok "目录结构检查通过"

BASE="-f deploy/docker-compose.prod.yml"

# ---------- 1. 安装 Docker ----------
install_docker() {
  info "未检测到 Docker，开始安装（约 1-3 分钟）…"
  if [ -f /etc/os-release ]; then . /etc/os-release; OS_ID="$ID"; else OS_ID="unknown"; fi

  case "$OS_ID" in
    ubuntu|debian|linuxmint)
      apt-get update -y
      apt-get install -y ca-certificates curl gnupg lsb-release
      install -m 0755 -d /etc/apt/keyrings
      curl -fsSL "https://download.docker.com/linux/${OS_ID}/gpg" \
        | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
      chmod a+r /etc/apt/keyrings/docker.gpg
      echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
https://download.docker.com/linux/${OS_ID} ${VERSION_CODENAME} stable" > /etc/apt/sources.list.d/docker.list
      apt-get update -y
      apt-get install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin
      ;;
    centos|rocky|almalinux|rhel|tencentos|opencloudos)
      yum install -y yum-utils
      yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
      yum install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin
      ;;
    *)
      warn "未知发行版（$OS_ID），尝试官方通用安装脚本…"
      curl -fsSL https://get.docker.com | sh
      ;;
  esac

  systemctl enable --now docker
  ok "Docker 安装完成"
}

docker --version >/dev/null 2>&1 && ok "Docker 已存在：$(docker --version)" || install_docker

if docker compose version >/dev/null 2>&1; then
  ok "docker compose 插件可用：$(docker compose version --short)"
else
  warn "缺少 compose 插件，尝试安装…"
  mkdir -p /usr/local/lib/docker/cli-plugins
  curl -SL "https://github.com/docker/compose/releases/latest/download/docker-compose-linux-$(uname -m)" \
    -o /usr/local/lib/docker/cli-plugins/docker-compose
  chmod +x /usr/local/lib/docker/cli-plugins/docker-compose
  docker compose version >/dev/null 2>&1 || fail "compose 插件安装失败，请手动安装"
  ok "compose 插件安装完成"
fi

systemctl is-active --quiet docker || systemctl start docker

# ---------- 2. 加载镜像 ----------
BUNDLE=""
for f in blog-bundle.tar.gz blog-image.tar.gz; do
  [ -f "$f" ] && BUNDLE="$f" && break
done
if docker images --format '{{.Repository}}:{{.Tag}}' | grep -qx 'blog:latest'; then
  ok "blog:latest 镜像已存在，跳过 load"
elif [ -z "$BUNDLE" ]; then
  info "未找到镜像包，开始从 GitHub 源码构建博客镜像…"
  docker build -t blog:latest .
  ok "博客镜像构建完成"
else
  info "加载镜像 $BUNDLE（约 1 分钟）…"
  docker load < "$BUNDLE"
  ok "镜像加载完成"
fi

# 轻量包才需要额外拉取 mysql/nginx（离线 bundle 里已包含）
if [ "$BUNDLE" = "blog-image.tar.gz" ]; then
  docker images --format '{{.Repository}}:{{.Tag}}' | grep -qx 'mysql:8.0'         || docker pull mysql:8.0
  docker images --format '{{.Repository}}:{{.Tag}}' | grep -qx 'nginx:1.27-alpine' || docker pull nginx:1.27-alpine
fi

# ---------- 3. 生成 .env ----------
rand() {
  if command -v openssl >/dev/null 2>&1; then openssl rand -hex "$1"
  else head -c "$1" /dev/urandom | base64 | tr -d '/+=' | head -c "$(( $1 * 2 ))"; fi
}

if [ ! -f deploy/.env ]; then
  info "首次部署，自动生成 deploy/.env（随机强密码）…"
  UP=$(rand 8); RP=$(rand 8); JS=$(rand 32)
  cat > deploy/.env <<EOF
# 自动生成于 $(date '+%F %T')，请勿提交 git

BLOG_JWT_SECRET=${JS}

DB_HOST=db-mysql
DB_PORT=3306
DB_NAME=blog
DB_USER=blog
DB_PASSWORD=${UP}

MYSQL_ROOT_PASSWORD=${RP}
MYSQL_PASSWORD=${UP}
DB_MODE=standalone
EOF
  chmod 600 deploy/.env
  ok "deploy/.env 已生成"
else
  chmod 600 deploy/.env
  ok "沿用已有 deploy/.env"
fi
set -a; . deploy/.env; set +a

# ---------- 4. 选择数据库部署方式 ----------
if [ -t 0 ] && [ -z "${DB_MODE:-}" ]; then
  echo
  echo "选择数据库的部署方式："
  echo "  1) 独立共享 MySQL 容器    推荐 · 多项目共用 · 更新博客不影响数据库"
  echo "  2) compose 自带 MySQL     简单 · 单项目自包含"
  echo "  3) 腾讯云托管 TencentDB   已购实例 · 自动备份高可用"
  read -r -p "请输入 1/2/3 [默认 1]: " CHOICE
  case "${CHOICE:-1}" in
    1) DB_MODE=standalone ;;
    2) DB_MODE=withdb ;;
    3) DB_MODE=manageddb ;;
    *) fail "无效选择：$CHOICE" ;;
  esac
fi

case "${DB_MODE:-standalone}" in
  standalone|withdb|manageddb) DB_MODE="${DB_MODE:-standalone}" ;;
  *) fail "未知的 DB_MODE=$DB_MODE（可选 standalone / withdb / manageddb）" ;;
esac

if grep -q '^DB_MODE=' deploy/.env; then
  sed -i "s/^DB_MODE=.*/DB_MODE=${DB_MODE}/" deploy/.env
else
  echo "DB_MODE=${DB_MODE}" >> deploy/.env
fi

case "$DB_MODE" in

  standalone)
    info "模式 1：独立共享 MySQL 容器"
    if ! docker ps -a --format '{{.Names}}' | grep -qx 'db-mysql'; then
      info "启动数据库实例 db-mysql（首次初始化约 30 秒）…"
      docker compose -f deploy/mysql-standalone.yml up -d
    else
      docker compose -f deploy/mysql-standalone.yml up -d
    fi
    for i in $(seq 1 60); do
      sleep 2
      [ "$(docker inspect -f '{{.State.Health.Status}}' db-mysql 2>/dev/null || true)" = "healthy" ] \
        && { ok "数据库已就绪"; break; }
      [ "$i" = "60" ] && { docker logs --tail 30 db-mysql; fail "数据库未就绪"; }
    done
    sed -i 's/^DB_HOST=.*/DB_HOST=db-mysql/' deploy/.env
    set -a; . deploy/.env; set +a
    info "启动博客（后端 + Nginx）…"
    docker compose $BASE up -d
    ;;

  withdb)
    info "模式 2：compose 自带 MySQL"
    sed -i 's/^DB_HOST=.*/DB_HOST=mysql/' deploy/.env
    set -a; . deploy/.env; set +a
    docker compose $BASE -f deploy/with-db.yml up -d
    ;;

  manageddb)
    info "模式 3：腾讯云托管 TencentDB"
    case "${DB_HOST:-}" in
      db-mysql|mysql|"") fail "请先在 deploy/.env 里把 DB_HOST 改成 TencentDB 内网地址（xxx.mysql.tencentcdb.com），并重跑本脚本" ;;
    esac
    [ -n "${DB_PASSWORD:-}" ] || fail "请在 deploy/.env 里填写 DB_PASSWORD"

    info "导入表结构到 ${DB_HOST}/${DB_NAME} …"
    if docker run --rm -i mysql:8.0 \
         sh -c "exec mysql -h'${DB_HOST}' -P'${DB_PORT}' -u'${DB_USER}' -p'${DB_PASSWORD}' '${DB_NAME}'" \
         < db/schema.sql; then
      ok "表结构导入完成"
    else
      warn "导入未成功（若表结构已存在属正常），请手动确认后再继续"
    fi
    docker compose $BASE up -d
    ;;
esac

# ---------- 5. 等待后端就绪 ----------
info "等待后端就绪…"
for i in $(seq 1 90); do
  sleep 2
  if curl -sf -o /dev/null "http://127.0.0.1/api/site" 2>/dev/null; then
    ok "后端已就绪"
    break
  fi
  if [ "$i" = "90" ]; then
    docker compose $BASE logs --tail 40 backend
    fail "后端未就绪，日志如上"
  fi
done

# ---------- 6. 首次设置管理员 ----------
bash deploy/init-admin.sh

# ---------- 7. 结果 ----------
PUBLIC_IP=$(curl -sf --max-time 3 http://metadata.tencentyun.com/latest/meta-data/public-ipv4 2>/dev/null \
         || curl -sf --max-time 3 ifconfig.me 2>/dev/null || echo "你的服务器公网IP")

if [ "$DB_MODE" = "withdb" ]; then
  START_CMD="docker compose $BASE -f deploy/with-db.yml up -d"
  STOP_CMD="docker compose $BASE -f deploy/with-db.yml down"
else
  START_CMD="docker compose $BASE up -d"
  STOP_CMD="docker compose $BASE down      # 数据库独立运行，不受影响"
fi

echo
cat <<EOF
${c_green}================ 部署完成 ================${c_reset}

  前台首页      http://${PUBLIC_IP}/
  后台登录      http://${PUBLIC_IP}/admin/login
  管理员账号    使用首次部署时设置的用户名和密码

  数据库模式    ${DB_MODE}
  数据库地址    ${DB_HOST}:${DB_PORT}/${DB_NAME}

  常用命令（目录 $ROOT_DIR）：
    查看日志    docker compose $BASE logs -f backend
    重启博客    ${START_CMD}
    停止博客    ${STOP_CMD}
    备份数据库  docker exec ${DB_HOST} mysqldump -uroot -p ${DB_NAME} > blog-\$(date +%F).sql

${c_green}=========================================${c_reset}
EOF

echo "接口自检："
printf "  /            %s\n" "$(curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1/)"
printf "  /api/site    %s\n" "$(curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1/api/site)"
printf "  /api/posts   %s\n" "$(curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1/api/posts)"
