/* 博客领域模型 —— 与 Drogon 后端返回的 JSON 字段保持一致 */

export type PostStatus = 'draft' | 'published' | 'hidden'

export interface Post {
  id: number
  /** URL 短标识，如 vue3-drogon-blog */
  slug: string
  title: string
  summary: string
  /** Markdown 原文 */
  content: string
  cover?: string
  tags: string[]
  category?: string
  musicId?: number | null
  isPinned?: boolean
  isHot?: boolean
  displayWeight?: number
  /** 发布状态：草稿 / 已发布 / 隐藏 */
  status: PostStatus
  /** ISO 8601 时间 */
  createdAt: string
  updatedAt: string
  /** 预计阅读时长（分钟） */
  readingMinutes: number
  views?: number
}

/** 列表项不含正文，减少传输体积 */
export type PostListItem = Omit<Post, 'content'>

export interface Paginated<T> {
  list: T[]
  total: number
  page: number
  pageSize: number
}

export interface TagInfo {
  name: string
  count: number
}

export interface ArchiveGroup {
  year: number
  posts: PostListItem[]
}

export interface SiteInfo {
  name: string
  owner: string
  title: string
  bio: string
  homeKicker?: string
  homeIntro?: string
  homeHeroTitle?: string
  homeTerminalTitle?: string
  homeMotto?: string
  homeMusicEnabled?: boolean
  homeMusicId?: number
  profileSectionTitle?: string
  avatar?: string
  location?: string
  email?: string
  github?: string
  social?: { label: string; url: string }[]
  skills?: { group: string; items: string[] }[]
  contentSections?: SiteContentSection[]
}

export interface SiteContentSection {
  id: string
  page: 'home' | 'about'
  slot: 'top' | 'middle' | 'bottom'
  title: string
  html: string
  visible: boolean
}

export interface TocItem {
  id: string
  text: string
  level: number
}

/* ---------- 评论 ---------- */

export type CommentStatus = 'pending' | 'approved' | 'rejected'

export interface Comment {
  id: number
  postId: number
  /** 冗余的文章标题，便于后台展示 */
  postTitle: string
  author: string
  email: string
  /** 网站地址，可空 */
  website?: string
  content: string
  status: CommentStatus
  /** 父评论 id，用于回复，可空 */
  parentId?: number
  createdAt: string
}

/* ---------- 友情链接 ---------- */

export interface FriendLink {
  id: number
  name: string
  url: string
  description?: string
  logo?: string
  /** 排序值，越小越靠前 */
  sortOrder: number
  visible: boolean
}
export interface ResourceItem {
  id: number
  title: string
  description: string
  image: string
  visible: boolean
  links: { label: string; url: string; code: string }[]
  linkLabels?: string[]
}

export interface MusicTrack {
  id: number
  title: string
  artist: string
  url: string
}

/* ---------- 后台 ---------- */

export interface LoginRequest {
  username: string
  password: string
}

export interface LoginResponse {
  token: string
  username: string
}

export interface AdminStats {
  postCount: number
  commentCount: number
  pendingCommentCount: number
  linkCount: number
  totalViews: number
  siteViews: number
  guestVisitors: number
  registeredVisitors: number
  readerAccounts: number
}

export type AnalyticsPeriod = 'day' | 'week' | 'month' | 'year'
export interface AnalyticsPoint { bucket: string; views: number; guests: number; readers: number; newAccounts: number }
export interface AnalyticsResult { period: AnalyticsPeriod; points: AnalyticsPoint[] }

/** 文章编辑时提交的载荷 */
export type PostInput = Omit<Post, 'id' | 'createdAt' | 'updatedAt' | 'readingMinutes' | 'views'>

/** 链接编辑载荷 */
export type FriendLinkInput = Omit<FriendLink, 'id'>
