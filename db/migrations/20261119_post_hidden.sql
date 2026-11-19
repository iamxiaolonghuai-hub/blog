ALTER TABLE posts
  MODIFY COLUMN status ENUM('draft', 'published', 'hidden') NOT NULL DEFAULT 'draft' COMMENT '草稿 / 已发布 / 隐藏';
