#!/usr/bin/env bash
# ============================================================
#  在博客共用的 MySQL 实例里，为新项目建库 + 建独立账号
#
#  用法：
#    ./db/create-database.sh myapp              # 库名 myapp，账号同名，密码随机
#    ./db/create-database.sh myapp myuser       # 自定义账号名
#    ./db/create-database.sh myapp myuser pass  # 自定义密码
#
#  执行后会打印可直接用的连接串。
# ============================================================
set -euo pipefail

DB_NAME="${1:-}"
DB_USER="${2:-}"
DB_PASS="${3:-}"

[ -n "$DB_NAME" ] || { echo "用法: $0 <数据库名> [账号名] [密码]"; exit 1; }
[ -n "$DB_USER" ] || DB_USER="$DB_NAME"
if [ -z "$DB_PASS" ]; then
  if command -v openssl >/dev/null 2>&1; then DB_PASS=$(openssl rand -hex 12);
  else DB_PASS=$(head -c 16 /dev/urandom | base64 | tr -d '/+=' | head -c 20); fi
fi

# 校验库名/账号名合法（防 SQL 注入式误操作）
[[ "$DB_NAME" =~ ^[A-Za-z0-9_]+$ ]] || { echo "库名只允许字母数字下划线"; exit 1; }
[[ "$DB_USER" =~ ^[A-Za-z0-9_]+$ ]] || { echo "账号名只允许字母数字下划线"; exit 1; }

CONTAINER="${MYSQL_CONTAINER:-}"
if [ -z "$CONTAINER" ]; then
  for c in db-mysql blog-mysql; do
    if docker ps --format '{{.Names}}' | grep -qx "$c"; then CONTAINER="$c"; break; fi
  done
fi
[ -n "$CONTAINER" ] || {
  echo "错误：没找到运行中的 MySQL 容器（db-mysql / blog-mysql）。"
  echo "  本地开发：      docker compose -p blog up -d"
  echo "  独立共享实例：  docker compose -f deploy/mysql-standalone.yml up -d"
  echo "  或手动指定容器：MYSQL_CONTAINER=你的容器名 $0 $DB_NAME"
  exit 1; }

can_login() {
  docker exec -e MYSQL_PWD="$1" "$CONTAINER" mysql -uroot -e "SELECT 1" >/dev/null 2>&1
}

ENV_PASS=""
[ -f deploy/.env ] && ENV_PASS=$(grep -E '^MYSQL_ROOT_PASSWORD=' deploy/.env | cut -d= -f2-)

ROOT_PASS=""
for cand in "${MYSQL_ROOT_PASSWORD:-}" "$ENV_PASS"; do
  [ -n "$cand" ] || continue
  if can_login "$cand"; then ROOT_PASS="$cand"; break; fi
done
[ -n "$ROOT_PASS" ] || { echo "错误：无法用已知密码登录容器 $CONTAINER 的 root。"; \
  echo "  可手动指定： MYSQL_ROOT_PASSWORD=你的root密码 $0 $DB_NAME"; exit 1; }

docker exec -e MYSQL_PWD="$ROOT_PASS" "$CONTAINER" mysql -uroot -e "
CREATE DATABASE IF NOT EXISTS \`$DB_NAME\`
  CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS '$DB_USER'@'%' IDENTIFIED BY '$DB_PASS';
ALTER USER '$DB_USER'@'%' IDENTIFIED BY '$DB_PASS';
GRANT ALL PRIVILEGES ON \`$DB_NAME\`.* TO '$DB_USER'@'%';
FLUSH PRIVILEGES;"

PUB_PORT=$(docker port "$CONTAINER" 3306/tcp 2>/dev/null | head -1 || true)

if [ -n "$PUB_PORT" ]; then
  HOST_HINT="127.0.0.1:${PUB_PORT#*:}（端口已映射到宿主机，本机任意程序可直接连）"
else
  HOST_HINT="容器内用主机名 ${CONTAINER}:3306；容器外需加入 blog-shared 共享网络"
fi

cat <<EOF

${DB_NAME} 建好了 ✅

  DATABASE_URL = mysql://${DB_USER}:${DB_PASS}@127.0.0.1:3306/${DB_NAME}

  主机        ${HOST_HINT}
  用户名      ${DB_USER}
  密码        ${DB_PASS}
  数据库      ${DB_NAME}
EOF

[ -n "$PUB_PORT" ] || cat <<'EOF'
  提示：该实例没有把 3306 映射到宿主机，容器外的程序请加入共享网络：
    networks:
      blog-shared:
        external: true
EOF

echo "  备份单库： docker exec $CONTAINER mysqldump -uroot -p $DB_NAME > ${DB_NAME}-\$(date +%F).sql"
