-- 已有博客库执行一次；只新增访问记录表，不修改原有文章浏览量。
CREATE TABLE IF NOT EXISTS visit_events (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  visitor_key VARCHAR(64) NOT NULL,
  reader_id BIGINT DEFAULT NULL,
  path VARCHAR(255) NOT NULL,
  created_at DATETIME NOT NULL,
  KEY idx_visit_created (created_at),
  KEY idx_visit_visitor (visitor_key, created_at),
  KEY idx_visit_reader (reader_id, created_at)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '站点页面访问记录（UTC）';
