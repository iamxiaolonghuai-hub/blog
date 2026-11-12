#!/usr/bin/env bash
# 在空 users 表中创建首个管理员；已有管理员时不修改任何账号。
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"
[ -f deploy/.env ] || { echo '请先生成 deploy/.env' >&2; exit 1; }
set -a
. deploy/.env
set +a

mysql_exec() {
  case "${DB_MODE:-standalone}" in
    standalone)
      docker exec -i -e "MYSQL_PWD=${MYSQL_ROOT_PASSWORD:?}" "${ADMIN_DB_CONTAINER:-db-mysql}" mysql -uroot "$DB_NAME" "$@" ;;
    withdb)
      docker exec -i -e "MYSQL_PWD=${MYSQL_ROOT_PASSWORD:?}" "${ADMIN_DB_CONTAINER:-blog-mysql}" mysql -uroot "$DB_NAME" "$@" ;;
    manageddb)
      docker run --rm -i --network host -e "MYSQL_PWD=${DB_PASSWORD:?}" mysql:8.0 \
        mysql -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" "$DB_NAME" "$@" ;;
    *) echo '未知数据库模式' >&2; return 1 ;;
  esac
}

count=$(mysql_exec -Nse 'SELECT COUNT(*) FROM users')
if [ "$count" != 0 ]; then
  echo '管理员已存在，保持现有账号不变。'
  exit 0
fi

if [ -t 0 ]; then
  read -r -p '设置管理员用户名（3-50 位字母、数字、下划线、点或连字符）: ' admin_username
  read -r -s -p '设置管理员密码（至少 8 位）: ' admin_password; echo
  read -r -s -p '再次输入密码: ' confirm_password; echo
  [ "$admin_password" = "$confirm_password" ] || { echo '两次密码不一致' >&2; exit 1; }
  unset confirm_password
else
  admin_username="${ADMIN_USERNAME:-}"
  admin_password="${ADMIN_PASSWORD:-}"
  [ -n "$admin_username" ] && [ -n "$admin_password" ] || {
    echo '非交互部署请临时提供 ADMIN_USERNAME 和 ADMIN_PASSWORD 环境变量' >&2; exit 1;
  }
fi

[[ "$admin_username" =~ ^[A-Za-z0-9_.-]{3,50}$ ]] || { echo '用户名格式无效' >&2; exit 1; }
[ "${#admin_password}" -ge 8 ] || { echo '密码至少 8 位' >&2; exit 1; }
admin_hash=$(printf '%s\n' "$admin_password" | docker run --rm -i --entrypoint /app/blog_server "${ADMIN_IMAGE:-blog:latest}" --hash-password)
unset admin_password ADMIN_PASSWORD
[[ "$admin_hash" =~ ^\$2b\$ ]] || { echo '密码哈希生成失败' >&2; exit 1; }

printf "INSERT INTO users(username,password_hash,nickname) VALUES('%s','%s','管理员');\n" \
  "$admin_username" "$admin_hash" | mysql_exec
unset admin_hash
echo "管理员 $admin_username 已创建。"
