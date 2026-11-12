import type { Comment, FriendLink, Post, SiteInfo } from './types'

// 把示例 Markdown 作为原始字符串打包进来
const rawModules = import.meta.glob('@/content/*.md', {
  query: '?raw',
  import: 'default',
  eager: true,
}) as Record<string, string>

function pick(keyword: string): string {
  const key = Object.keys(rawModules).find((k) => k.includes(keyword))
  if (!key) throw new Error(`missing mock content: ${keyword}`)
  return rawModules[key]
}

function readingMinutes(content: string): number {
  // 中文按字符数粗略估算，约 400 字/分钟
  return Math.max(1, Math.round(content.length / 400))
}

const vueDrogon = pick('vue3-drogon-blog')
const drogonNotes = pick('cpp-drogon-notes')
const tsTips = pick('typescript-tips')
const dockerWp = pick('docker-debian-wordpress')

export const mockPosts: Post[] = [
  {
    id: 1,
    slug: 'vue3-drogon-blog',
    title: '用 Vue3 和 Drogon 搭建个人博客',
    summary:
      '前端用 Vue3 + TypeScript，后端用 C++ Drogon，文章以 Markdown 存储。记录这个前后端分离博客的完整架构与关键实现。',
    content: vueDrogon,
    tags: ['vue', 'c++', '全栈'],
    category: '工程实践',
    status: 'published',
    createdAt: '2026-10-20T09:30:00+08:00',
    updatedAt: '2026-10-20T09:30:00+08:00',
    readingMinutes: readingMinutes(vueDrogon),
    views: 128,
  },
  {
    id: 2,
    slug: 'cpp-drogon-notes',
    title: 'Drogon 学习笔记：高性能 C++ Web 框架',
    summary:
      'Drogon 基于 C++17，异步非阻塞、支持协程并自带 ORM。整理安装、协程、路由和静态文件托管的核心用法。',
    content: drogonNotes,
    tags: ['c++', '后端'],
    category: '后端',
    status: 'published',
    createdAt: '2026-10-12T14:10:00+08:00',
    updatedAt: '2026-10-13T20:00:00+08:00',
    readingMinutes: readingMinutes(drogonNotes),
    views: 86,
  },
  {
    id: 3,
    slug: 'typescript-tips',
    title: 'TypeScript 实战中高频的几个类型技巧',
    summary:
      '接口类型、可辨识联合、keyof/typeof、泛型工具函数，以及在 Vue 组件里如何用好类型，覆盖日常绝大多数场景。',
    content: tsTips,
    tags: ['typescript', '前端'],
    category: '前端',
    status: 'published',
    createdAt: '2026-10-05T11:00:00+08:00',
    updatedAt: '2026-10-05T11:00:00+08:00',
    readingMinutes: readingMinutes(tsTips),
    views: 204,
  },
  {
    id: 4,
    slug: 'docker-debian-wordpress',
    title: '在 Debian 上用 Docker 部署 WordPress 完整教程',
    summary:
      '面向拥有 Linux 云服务器的用户：从更新系统、安装 Docker，到用 docker-compose 一键编排 MySQL + WordPress，并修改上传限制。',
    content: dockerWp,
    tags: ['docker', 'linux', '部署'],
    category: '运维',
    status: 'published',
    createdAt: '2026-09-28T16:20:00+08:00',
    updatedAt: '2026-09-29T10:00:00+08:00',
    readingMinutes: readingMinutes(dockerWp),
    views: 312,
  },
]

export const mockSite: SiteInfo = {
  name: '龙淮的札记',
  owner: '肖龙淮',
  title: '系统、代码与思考',
  bio: '一个系统程序员，写 C++ 后端，也写 Vue 前端。把踩过的坑和想清楚的问题记在这里。',
  homeKicker: '个人博客 · personal blog',
  homeIntro: '肖龙淮 · C++ backend / Vue frontend',
  homeHeroTitle: '在终端与代码之间，\n记录构建系统的过程。',
  homeTerminalTitle: 'zsh — xiaolonghuai',
  homeMotto: '把想清楚的问题，写下来。',
  profileSectionTitle: '个人方向与兴趣',
  location: '武汉',
  email: 'hello@example.com',
  github: 'https://github.com/yourname',
  social: [
    { label: 'GitHub', url: 'https://github.com/yourname' },
    { label: 'Email', url: 'mailto:hello@example.com' },
  ],
}

/* ---------- 评论（覆盖三种审核状态） ---------- */

export const mockComments: Comment[] = [
  {
    id: 1,
    postId: 1,
    postTitle: '用 Vue3 和 Drogon 搭建个人博客',
    author: '张三',
    email: 'zhangsan@example.com',
    website: 'https://zhangsan.dev',
    content: '前后端这个组合很有意思，Markdown 渲染放前端的思路学到了，请问评论功能后面会做吗？',
    status: 'approved',
    createdAt: '2026-10-21T10:20:00+08:00',
  },
  {
    id: 2,
    postId: 4,
    postTitle: '在 Debian 上用 Docker 部署 WordPress 完整教程',
    author: '运维小白',
    email: 'newbie@example.com',
    content: '跟着步骤一次成功了，感谢！docker compose 空格写法那个坑确实踩了。',
    status: 'pending',
    createdAt: '2026-10-22T19:45:00+08:00',
  },
  {
    id: 3,
    postId: 2,
    postTitle: 'Drogon 学习笔记：高性能 C++ Web 框架',
    author: 'spam-bot',
    email: 'spam@spam.com',
    content: '免费发广告，点击链接领取优惠……',
    status: 'rejected',
    createdAt: '2026-10-22T03:10:00+08:00',
  },
  {
    id: 4,
    postId: 3,
    postTitle: 'TypeScript 实战中高频的几个类型技巧',
    author: '李四',
    email: 'lisi@example.com',
    content: '可辨识联合那段写得很清楚，配合 switch 用确实安全。',
    status: 'pending',
    createdAt: '2026-10-23T08:30:00+08:00',
  },
]

/* ---------- 友情链接 ---------- */

export const mockLinks: FriendLink[] = [
  {
    id: 1,
    name: 'Drogon 官网',
    url: 'https://drogon.org',
    description: '高性能 C++ HTTP 应用框架',
    sortOrder: 1,
    visible: true,
  },
  {
    id: 2,
    name: 'Vue 官方文档',
    url: 'https://cn.vuejs.org',
    description: '渐进式 JavaScript 框架',
    sortOrder: 2,
    visible: true,
  },
  {
    id: 3,
    name: '待整理的友链',
    url: 'https://example.com',
    description: '暂时隐藏',
    sortOrder: 9,
    visible: false,
  },
]

/* ---------- 管理员账号（演示用，后端就绪后由 Drogon 校验） ---------- */

export const adminAccount = {
  username: 'admin',
  password: 'demo-only-password',
}

export const mockToken = 'mock-jwt-token-demo'
