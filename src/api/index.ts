import { get, post, upload } from './request'
import type { Comment, FriendLink, MusicTrack, Post, PostListItem, SiteInfo, TagInfo, ResourceItem } from './types'

/**
 * 仅当显式设置 VITE_USE_MOCK=true 时才回退到本地示例数据。
 * 正式模式下后端错误（4xx/5xx/网络异常）会正常抛出，
 * 不再用 mock 数据静默掩盖故障。
 */
const FORCE_MOCK = import.meta.env.VITE_USE_MOCK === 'true'

async function withFallback<T>(
  fn: () => Promise<T>,
  fallback: () => Promise<T>,
): Promise<T> {
  if (FORCE_MOCK) return fallback()
  return fn()
}

// mock 数据按需加载，避免示例文章被打进生产包
async function localPosts(): Promise<Post[]> {
  const { postsStore } = await import('./store')
  return postsStore.filter((p) => p.status === 'published')
}

async function localSite(): Promise<SiteInfo> {
  const { mockSite } = await import('./mock')
  return mockSite
}

function toListItem(p: Post): PostListItem {
  return {
    id: p.id,
    slug: p.slug,
    title: p.title,
    summary: p.summary,
    cover: p.cover,
    tags: p.tags,
    category: p.category,
    status: p.status,
    createdAt: p.createdAt,
    updatedAt: p.updatedAt,
    readingMinutes: p.readingMinutes,
    views: p.views,
  }
}

function buildTags(posts: PostListItem[]): TagInfo[] {
  const map = new Map<string, number>()
  posts.forEach((p) => p.tags.forEach((t) => map.set(t, (map.get(t) ?? 0) + 1)))
  return [...map.entries()]
    .map(([name, count]) => ({ name, count }))
    .sort((a, b) => b.count - a.count || a.name.localeCompare(b.name))
}

async function fetchPublishedPosts(): Promise<PostListItem[]> {
  const posts = await withFallback<Post[]>(() => get<Post[]>('/posts'), localPosts)
  return posts.map(toListItem)
}

export const blogApi = {
  listMusic(): Promise<MusicTrack[]> { return get('/music') },
  listResources(): Promise<ResourceItem[]> { return get<ResourceItem[]>('/resources') },
  resourceAccess(id: number): Promise<{ links: ResourceItem['links'] }> {
    return get(`/resources/${id}/access`)
  },
  /** 文章列表，可按标签过滤（前台只显示已发布） */
  listPosts(tag?: string): Promise<PostListItem[]> {
    return fetchPublishedPosts().then((list) =>
      tag ? list.filter((p) => p.tags.includes(tag)) : list,
    )
  },

  /** 文章详情（含 Markdown 原文） */
  getPost(slug: string): Promise<Post> {
    return withFallback<Post>(
      () => get<Post>(`/posts/${encodeURIComponent(slug)}`),
      async () => {
        const { postsStore } = await import('./store')
        const p = postsStore.find(
          (x) => x.slug === slug && x.status === 'published',
        )
        if (!p) throw new Error('post not found')
        return p
      },
    )
  },

  /** 标签及计数（后端 /tags 聚合接口，异常时从文章列表派生） */
  async listTags(): Promise<TagInfo[]> {
    try {
      return await get<TagInfo[]>('/tags')
    } catch {
      return buildTags(await fetchPublishedPosts())
    }
  },

  /** 站点信息 */
  getSite(): Promise<SiteInfo> {
    return withFallback<SiteInfo>(() => get<SiteInfo>('/site'), localSite)
  },

  /** 获取某篇文章已通过的评论 */
  listComments(slug: string): Promise<Comment[]> {
    return withFallback<Comment[]>(
      () => get<Comment[]>(`/posts/${encodeURIComponent(slug)}/comments`),
      async () => {
        const { commentsStore } = await import('./store')
        return commentsStore.filter((c) => c.status === 'approved')
      },
    )
  },

  /** 访客提交评论（默认待审） */
  submitComment(
    slug: string,
    body: { email?: string; website?: string; content: string; parentId?: number },
  ): Promise<Comment> {
    return post<Comment>(
      `/posts/${encodeURIComponent(slug)}/comments`,
      body,
    )
  },

  /** 评论图片上传（前台访客可上传），返回可访问 URL */
  uploadCommentImage(file: File): Promise<{ url: string }> {
    return upload<{ url: string }>('/upload/comment', file)
  },
  readerLogin(username: string, password: string): Promise<{ token: string; username: string }> {
    return post('/readers/login', { username, password })
  },
  readerRegister(username: string, password: string): Promise<{ token: string; username: string }> {
    return post('/readers/register', { username, password })
  },

  /** 友情链接（仅 visible 的，后台可维护） */
  listLinks(): Promise<FriendLink[]> {
    return get<FriendLink[]>('/links')
  },
}
