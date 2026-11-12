import { mockComments, mockLinks, mockPosts } from './mock'
import type { Comment, FriendLink, Post } from './types'

/**
 * 内存数据层：会话期间的增删改都作用在这里。
 * 后端就绪后，这些操作会替换为真实 HTTP 请求，逻辑不变。
 */
export const postsStore: Post[] = [...mockPosts]
export const commentsStore: Comment[] = [...mockComments]
export const linksStore: FriendLink[] = [...mockLinks]

export function nextPostId(): number {
  return Math.max(0, ...postsStore.map((p) => p.id)) + 1
}
export function nextCommentId(): number {
  return Math.max(0, ...commentsStore.map((c) => c.id)) + 1
}
export function nextLinkId(): number {
  return Math.max(0, ...linksStore.map((l) => l.id)) + 1
}

/** 模拟网络延迟，让 loading 态更真实 */
export function networkDelay(ms = 220): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms))
}
