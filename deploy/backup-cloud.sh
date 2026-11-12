#!/usr/bin/env bash
set -euo pipefail
cd /opt/blog
set -a
. deploy/.env
set +a
[ "${DB_HOST:-}" = db-mysql ] && [ "${DB_NAME:-}" = blog ] || {
  echo 'Unexpected database target; refusing backup' >&2; exit 1;
}
stamp=$(date +%Y%m%d-%H%M%S)
backup="/opt/blog-backups/$stamp"
mkdir -p "$backup"
chmod 700 "$backup"
cp deploy/.env "$backup/deploy.env"
docker exec -e MYSQL_PWD="$MYSQL_ROOT_PASSWORD" db-mysql \
  mysqldump -uroot --single-transaction --routines --triggers blog > "$backup/blog.sql"
tar -C /var/lib/docker/volumes/blog_blog_uploads/_data -czf "$backup/uploads.tar.gz" .
sha256sum "$backup/blog.sql" "$backup/uploads.tar.gz" > "$backup/SHA256SUMS"
test -s "$backup/blog.sql"
echo "BACKUP=$backup"
du -h "$backup/blog.sql" "$backup/uploads.tar.gz"
