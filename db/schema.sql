-- =====================================================================
-- 个人博客数据库表结构（MySQL 8.0+ / InnoDB / utf8mb4）
-- 字段命名 snake_case；前端对应字段为 camelCase，Drogon 序列化时转换。
-- =====================================================================

SET NAMES utf8mb4;
SET CHARACTER SET utf8mb4;

CREATE DATABASE IF NOT EXISTS blog
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE blog;

CREATE TABLE IF NOT EXISTS site_settings (
  id TINYINT NOT NULL PRIMARY KEY,
  data JSON NOT NULL
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

CREATE TABLE IF NOT EXISTS resources (
  id BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  data JSON NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ---------------------------------------------------------------------
-- 1. 管理员用户（对应前端 LoginRequest / 登录）
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
  id            BIGINT       NOT NULL AUTO_INCREMENT,
  username      VARCHAR(50)  NOT NULL COMMENT '登录用户名',
  password_hash VARCHAR(255) NOT NULL COMMENT 'bcrypt 哈希后的密码',
  nickname      VARCHAR(50)  DEFAULT NULL COMMENT '昵称（默认管理员）',
  created_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_users_username (username)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '管理员用户';

CREATE TABLE IF NOT EXISTS reader_accounts (
  id BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(50) NOT NULL UNIQUE,
  password_hash VARCHAR(255) NOT NULL,
  registration_ip VARCHAR(45) DEFAULT NULL COMMENT '注册时的客户端 IP',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '评论账号';

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

CREATE TABLE IF NOT EXISTS music_tracks (
  id BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  title VARCHAR(200) NOT NULL,
  artist VARCHAR(120) NOT NULL DEFAULT '',
  file_url VARCHAR(500) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '博客音乐库';

-- ---------------------------------------------------------------------
-- 2. 文章（对应前端 Post / PostInput）
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS posts (
  id           BIGINT       NOT NULL AUTO_INCREMENT,
  slug         VARCHAR(120) NOT NULL COMMENT 'URL 唯一标识，如 vue3-drogon-blog',
  title        VARCHAR(200) NOT NULL COMMENT '标题',
  summary      VARCHAR(500) DEFAULT NULL COMMENT '摘要',
  content      MEDIUMTEXT   NOT NULL COMMENT 'Markdown 原文',
  cover        VARCHAR(255) DEFAULT NULL COMMENT '封面图地址',
  category     VARCHAR(50)  DEFAULT NULL COMMENT '分类',
  music_id     BIGINT       DEFAULT NULL COMMENT '文章背景音乐',
  is_pinned    BOOLEAN      NOT NULL DEFAULT FALSE COMMENT '首页和列表置顶',
  is_hot       BOOLEAN      NOT NULL DEFAULT FALSE COMMENT '热门标记',
  display_weight INT        NOT NULL DEFAULT 0 COMMENT '展示权重，越大越靠前',
  status       ENUM('draft', 'published', 'hidden') NOT NULL DEFAULT 'draft' COMMENT '草稿 / 已发布 / 隐藏',
  views        INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '浏览量',
  published_at DATETIME     DEFAULT NULL COMMENT '发布时间',
  created_at   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_posts_slug (slug),
  KEY idx_posts_status (status),
  KEY idx_posts_category (category),
  KEY idx_posts_published_at (published_at),
  CONSTRAINT fk_posts_music FOREIGN KEY (music_id) REFERENCES music_tracks(id) ON DELETE SET NULL
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '文章';

-- ---------------------------------------------------------------------
-- 3. 标签（对应前端 Post.tags，规范化为独立表）
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS tags (
  id         BIGINT      NOT NULL AUTO_INCREMENT,
  name       VARCHAR(50) NOT NULL COMMENT '标签名',
  slug       VARCHAR(60) NOT NULL COMMENT '标签 URL 标识',
  created_at DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_tags_name (name),
  UNIQUE KEY uk_tags_slug (slug)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '标签';

-- 文章-标签 多对多关联
CREATE TABLE IF NOT EXISTS post_tags (
  post_id BIGINT NOT NULL,
  tag_id  BIGINT NOT NULL,
  PRIMARY KEY (post_id, tag_id),
  KEY idx_pt_tag (tag_id),
  CONSTRAINT fk_pt_post FOREIGN KEY (post_id) REFERENCES posts (id) ON DELETE CASCADE,
  CONSTRAINT fk_pt_tag  FOREIGN KEY (tag_id)  REFERENCES tags (id)  ON DELETE CASCADE
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '文章标签关联';

-- ---------------------------------------------------------------------
-- 4. 评论（对应前端 Comment）
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS comments (
  id         BIGINT       NOT NULL AUTO_INCREMENT,
  post_id    BIGINT       NOT NULL COMMENT '所属文章',
  parent_id  BIGINT       DEFAULT NULL COMMENT '父评论，用于回复（管理员回复）',
  author     VARCHAR(100) NOT NULL COMMENT '评论者昵称',
  email      VARCHAR(100) NOT NULL COMMENT '邮箱',
  website    VARCHAR(255) DEFAULT NULL COMMENT '个人网站',
  content    TEXT         NOT NULL COMMENT '评论内容',
  status     ENUM('pending', 'approved', 'rejected') NOT NULL DEFAULT 'pending'
             COMMENT '待审 / 已通过 / 已拒绝',
  created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_comments_post (post_id),
  KEY idx_comments_parent (parent_id),
  KEY idx_comments_status (status),
  KEY idx_comments_created (created_at),
  CONSTRAINT fk_comments_post   FOREIGN KEY (post_id)   REFERENCES posts (id) ON DELETE CASCADE,
  CONSTRAINT fk_comments_parent FOREIGN KEY (parent_id) REFERENCES comments (id) ON DELETE CASCADE
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '评论';

-- ---------------------------------------------------------------------
-- 5. 友情链接（对应前端 FriendLink / FriendLinkInput）
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS links (
  id          BIGINT       NOT NULL AUTO_INCREMENT,
  name        VARCHAR(100) NOT NULL COMMENT '站点名称',
  url         VARCHAR(255) NOT NULL COMMENT '站点地址',
  description VARCHAR(255) DEFAULT NULL COMMENT '描述',
  logo        VARCHAR(255) DEFAULT NULL COMMENT 'logo 地址',
  sort_order  INT          NOT NULL DEFAULT 0 COMMENT '排序，越小越靠前',
  visible     TINYINT(1)   NOT NULL DEFAULT 1 COMMENT '是否前台可见',
  created_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_links_name (name),
  KEY idx_links_visible (visible),
  KEY idx_links_sort (sort_order)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '友情链接';

-- =====================================================================
-- 初始数据
-- =====================================================================

-- 管理员在首次部署时由 deploy/init-admin.sh 设置，不预置公开密码。

-- 示例友情链接（幂等：重复执行不会插入重复行）
-- 注意：links 表有 uk_links_name 唯一键，配合 ON DUPLICATE KEY 才幂等。
INSERT INTO links (name, url, description, sort_order, visible) VALUES
  ('Drogon 官网', 'https://drogon.org',     '高性能 C++ HTTP 应用框架', 1, 1),
  ('Vue 官方文档', 'https://cn.vuejs.org',  '渐进式 JavaScript 框架',   2, 1),
  ('待整理的友链', 'https://example.com',   '暂时隐藏',                 9, 0)
AS new
ON DUPLICATE KEY UPDATE
  url = new.url,
  description = new.description,
  sort_order = new.sort_order,
  visible = new.visible;
