import { del, get, post, put, upload, http } from './request'
import type { AxiosProgressEvent } from 'axios'
import type {
  AdminStats,
  AnalyticsPeriod,
  AnalyticsResult,
  Comment,
  CommentStatus,
  FriendLink,
  FriendLinkInput,
  LoginRequest,
  LoginResponse,
  Post,
  PostInput,
  SiteInfo,
  ResourceItem,
  MusicTrack,
} from './types'

const FORCE_MOCK = import.meta.env.VITE_USE_MOCK === 'true'
export interface BackupSettings {
  enabled: boolean
  time: string
  retentionDays: number
  remoteEnabled: boolean
  remoteHost: string
  remoteUser: string
  remotePath: string
  remotePort: number
}
export interface BackupInfo {
  settings: Partial<BackupSettings>
  status: { state?: string; action?: string; message?: string; finishedAt?: string }
  files: { name: string; bytes: number }[]
  remotePublicKey: string
}

// 本地 mock 实现按需加载，不进生产包
let localCache: Promise<typeof import('./adminLocal')> | null = null
function localMod() {
  localCache ??= import('./adminLocal')
  return localCache
}

/**
 * 仅当显式设置 VITE_USE_MOCK=true 时走本地内存数据；
 * 正式模式下后端错误（4xx/5xx/网络异常）正常抛出，
 * 不再出现"保存失败却提示成功"的假成功。
 */
async function localOrRemote<T>(
  local: (mod: typeof import('./adminLocal')) => Promise<T>,
  remote: () => Promise<T>,
): Promise<T> {
  if (FORCE_MOCK) return local(await localMod())
  return remote()
}

export const adminApi = {
  listMusic(): Promise<MusicTrack[]> { return get('/music') },
  createMusic(input: Omit<MusicTrack, 'id'>): Promise<MusicTrack> { return post('/admin/music', input) },
  updateMusic(id: number, input: Omit<MusicTrack, 'id'>): Promise<MusicTrack> { return put(`/admin/music/${id}`, input) },
  deleteMusic(id: number): Promise<void> { return del(`/admin/music/${id}`) },
  uploadMusic(file: File, onProgress?: (event: AxiosProgressEvent) => void): Promise<{ url: string }> { return upload('/admin/upload/music', file, 'file', onProgress) },
  analytics(period: AnalyticsPeriod): Promise<AnalyticsResult> { return get('/admin/analytics', { period }) },
  changePassword(currentPassword: string, newPassword: string): Promise<{ message: string }> {
    return post('/admin/change-password', { currentPassword, newPassword })
  },
  backupInfo(): Promise<BackupInfo> { return get('/admin/backups') },
  saveBackupSettings(input: BackupSettings): Promise<BackupSettings> { return put('/admin/backups/settings', input) },
  runBackup(): Promise<{ message: string }> { return post('/admin/backups/run') },
  downloadBackup(name: string): Promise<Blob> { return http.get(`/admin/backups/${encodeURIComponent(name)}`, { responseType: 'blob', timeout: 120000 }) as unknown as Promise<Blob> },
  restoreBackup(file: File): Promise<{ message: string }> {
    const form = new FormData()
    form.append('file', file)
    return http.post('/admin/backups/restore', form, { timeout: 600000 }) as unknown as Promise<{ message: string }>
  },
  listResources(): Promise<ResourceItem[]> { return get('/admin/resources') },
  createResource(input: Omit<ResourceItem, 'id'>): Promise<ResourceItem> { return post('/admin/resources', input) },
  updateResource(id: number, input: Omit<ResourceItem, 'id'>): Promise<ResourceItem> { return put(`/admin/resources/${id}`, input) },
  deleteResource(id: number): Promise<void> { return del(`/admin/resources/${id}`) },
  uploadResourceImage(file: File): Promise<{ url: string }> { return upload('/admin/upload/resource-image', file) },
  uploadAvatar(file: File): Promise<{ url: string }> { return upload('/admin/upload/avatar', file) },
  updateSite(input: SiteInfo): Promise<SiteInfo> {
    return put<SiteInfo>('/admin/site', input)
  },
  updateHomeMusic(enabled: boolean, musicId: number): Promise<SiteInfo> {
    return put<SiteInfo>('/admin/site', { homeMusicEnabled: enabled, homeMusicId: musicId })
  },
  /* ---------- 登录 ---------- */
  login(req: LoginRequest): Promise<LoginResponse> {
    return localOrRemote(
      (m) => m.loginLocal(req),
      () => post<LoginResponse>('/auth/login', req),
    )
  },

  /* ---------- 仪表盘 ---------- */
  stats(): Promise<AdminStats> {
    return localOrRemote(
      (m) => m.statsLocal(),
      () => get<AdminStats>('/admin/stats'),
    )
  },

  /* ---------- 文章 ---------- */
  listAllPosts(): Promise<Post[]> {
    return localOrRemote(
      (m) => m.listAllPostsLocal(),
      () => get<Post[]>('/admin/posts'),
    )
  },

  /** 单篇文章（含正文），编辑页使用 */
  getPost(id: number): Promise<Post> {
    return localOrRemote(
      async (m) => {
        const p = await m.findPostLocal(id)
        if (!p) throw new Error('post not found')
        return p
      },
      () => get<Post>(`/admin/posts/${id}`),
    )
  },

  createPost(input: PostInput): Promise<Post> {
    return localOrRemote(
      (m) => m.createPostLocal(input),
      () => post<Post>('/admin/posts', input),
    )
  },

  updatePost(id: number, input: PostInput): Promise<Post> {
    return localOrRemote(
      (m) => m.updatePostLocal(id, input),
      () => put<Post>(`/admin/posts/${id}`, input),
    )
  },

  updatePostDisplay(id: number, input: { isPinned: boolean; isHot: boolean; displayWeight: number }): Promise<Post> {
    return localOrRemote(
      (m) => m.updatePostLocal(id, input),
      () => put<Post>(`/admin/posts/${id}/display`, input),
    )
  },

  deletePost(id: number): Promise<void> {
    return localOrRemote(
      (m) => m.deletePostLocal(id),
      () => del<void>(`/admin/posts/${id}`),
    )
  },

  /* ---------- 评论 ---------- */
  listComments(status?: CommentStatus): Promise<Comment[]> {
    return localOrRemote(
      (m) => m.listCommentsLocal(status),
      async () => {
        // 兼容旧版后端：带 status 参数的接口存在协程临时对象悬空问题。
        const comments = await get<Comment[]>('/admin/comments')
        return status ? comments.filter((comment) => comment.status === status) : comments
      },
    )
  },

  setCommentStatus(id: number, status: CommentStatus): Promise<Comment> {
    return localOrRemote(
      (m) => m.setCommentStatusLocal(id, status),
      () => post<Comment>(`/admin/comments/${id}/status`, { status }),
    )
  },

  replyComment(id: number, content: string): Promise<Comment> {
    return localOrRemote(
      (m) => m.replyCommentLocal(id, content),
      () => post<Comment>(`/admin/comments/${id}/reply`, { content }),
    )
  },

  deleteComment(id: number): Promise<void> {
    return localOrRemote(
      (m) => m.deleteCommentLocal(id),
      () => del<void>(`/admin/comments/${id}`),
    )
  },

  /* ---------- 友情链接 ---------- */
  listLinks(): Promise<FriendLink[]> {
    return localOrRemote(
      (m) => m.listLinksLocal(),
      () => get<FriendLink[]>('/admin/links'),
    )
  },

  createLink(input: FriendLinkInput): Promise<FriendLink> {
    return localOrRemote(
      (m) => m.createLinkLocal(input),
      () => post<FriendLink>('/admin/links', input),
    )
  },

  updateLink(id: number, input: FriendLinkInput): Promise<FriendLink> {
    return localOrRemote(
      (m) => m.updateLinkLocal(id, input),
      () => put<FriendLink>(`/admin/links/${id}`, input),
    )
  },

  deleteLink(id: number): Promise<void> {
    return localOrRemote(
      (m) => m.deleteLinkLocal(id),
      () => del<void>(`/admin/links/${id}`),
    )
  },

  /* ---------- 文件上传 ---------- */
  /** 文章封面，返回可访问 URL */
  uploadCover(file: File): Promise<{ url: string }> {
    return upload<{ url: string }>('/admin/upload/cover', file)
  },

  /** 链接 logo，返回可访问 URL */
  uploadLogo(file: File): Promise<{ url: string }> {
    return upload<{ url: string }>('/admin/upload/logo', file)
  },
  uploadArticleImage(file: File): Promise<{ url: string }> {
    return upload<{ url: string }>('/admin/upload/article-image', file)
  },
  uploadAttachment(file: File): Promise<{ url: string }> {
    return upload<{ url: string }>('/admin/upload/attachment', file)
  },
}
