-- 已有博客库执行一次；保留现有读者账号。
ALTER TABLE reader_accounts
  ADD COLUMN registration_ip VARCHAR(45) DEFAULT NULL COMMENT '注册时的客户端 IP' AFTER password_hash;
