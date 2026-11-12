import type { Comment, FriendLink, Post } from './types'
import {
  commentsStore,
  linksStore,
  networkDelay,
  nextCommentId,
  nextLinkId,
  nextPostId,
  postsStore,
} from './store'
import { adminAccount, mockToken } from './mock'

function nowIso(): string {
  return new Date().toISOString()
}

function minutesOf(content: string): number {
  return Math.max(1, Math.round(content.length / 400))
}

/* ----------
 * 以下为 VITE_USE_MOCK=true 时的本地内存实现。
 * 单独成模块 + 由 admin.ts 动态 import，保证不会进生产包。
 * ---------- */

export async function loginLocal(req: {
  username: string
  password: string
}): Promise<{ token: string; username: string }> {
  await networkDelay()
  if (
    req.username === adminAccount.username &&
    req.password === adminAccount.password
  ) {
    return { token: mockToken, username: req.username }
  }
  throw new Error('用户名或密码错误')
}

export async function statsLocal() {
  await networkDelay()
  return {
    postCount: postsStore.filter((p) => p.status === 'published').length,
    commentCount: commentsStore.length,
    pendingCommentCount: commentsStore.filter((c) => c.status === 'pending')
      .length,
    linkCount: linksStore.filter((l) => l.visible).length,
    totalViews: postsStore.reduce((s, p) => s + (p.views ?? 0), 0),
    siteViews: 0,
    guestVisitors: 0,
    registeredVisitors: 0,
    readerAccounts: 0,
  }
}

export async function listAllPostsLocal(): Promise<Post[]> {
  await networkDelay()
  return [...postsStore].sort((a, b) =>
    b.updatedAt.localeCompare(a.updatedAt),
  )
}

export async function findPostLocal(id: number): Promise<Post | undefined> {
  await networkDelay()
  return postsStore.find((p) => p.id === id)
}

export async function createPostLocal(input: Omit<Post, 'id' | 'createdAt' | 'updatedAt' | 'views' | 'readingMinutes'>): Promise<Post> {
  await networkDelay()
  const post: Post = {
    ...(input as Post),
    id: nextPostId(),
    readingMinutes: minutesOf(input.content),
    views: 0,
    createdAt: nowIso(),
    updatedAt: nowIso(),
  }
  postsStore.unshift(post)
  return post
}

export async function updatePostLocal(
  id: number,
  input: Partial<Post>,
): Promise<Post> {
  await networkDelay()
  const idx = postsStore.findIndex((p) => p.id === id)
  if (idx < 0) throw new Error('post not found')
  postsStore[idx] = {
    ...postsStore[idx],
    ...input,
    id,
    readingMinutes: minutesOf(input.content ?? postsStore[idx].content),
    updatedAt: nowIso(),
  }
  return postsStore[idx]
}

export async function deletePostLocal(id: number): Promise<void> {
  await networkDelay()
  const idx = postsStore.findIndex((p) => p.id === id)
  if (idx >= 0) postsStore.splice(idx, 1)
}

export async function listCommentsLocal(
  status?: string,
): Promise<Comment[]> {
  await networkDelay()
  const list = status
    ? commentsStore.filter((c) => c.status === status)
    : commentsStore
  return [...list].sort((a, b) => b.createdAt.localeCompare(a.createdAt))
}

export async function setCommentStatusLocal(
  id: number,
  status: string,
): Promise<Comment> {
  await networkDelay()
  const c = commentsStore.find((x) => x.id === id)
  if (!c) throw new Error('comment not found')
  c.status = status as Comment['status']
  return c
}

export async function replyCommentLocal(
  id: number,
  content: string,
): Promise<Comment> {
  await networkDelay()
  const parent = commentsStore.find((x) => x.id === id)
  if (!parent) throw new Error('comment not found')
  const reply: Comment = {
    id: nextCommentId(),
    postId: parent.postId,
    postTitle: parent.postTitle,
    author: '管理员',
    email: 'admin',
    content,
    status: 'approved',
    parentId: parent.id,
    createdAt: nowIso(),
  }
  commentsStore.unshift(reply)
  if (parent.status === 'pending') parent.status = 'approved'
  return reply
}

export async function deleteCommentLocal(id: number): Promise<void> {
  await networkDelay()
  const idx = commentsStore.findIndex((c) => c.id === id)
  if (idx >= 0) commentsStore.splice(idx, 1)
}

export async function listLinksLocal(): Promise<FriendLink[]> {
  await networkDelay()
  return [...linksStore].sort((a, b) => a.sortOrder - b.sortOrder)
}

export async function createLinkLocal(
  input: Omit<FriendLink, 'id'>,
): Promise<FriendLink> {
  await networkDelay()
  const link: FriendLink = { ...input, id: nextLinkId() }
  linksStore.push(link)
  return link
}

export async function updateLinkLocal(
  id: number,
  input: Partial<FriendLink>,
): Promise<FriendLink> {
  await networkDelay()
  const idx = linksStore.findIndex((l) => l.id === id)
  if (idx < 0) throw new Error('link not found')
  linksStore[idx] = { ...linksStore[idx], ...input, id }
  return linksStore[idx]
}

export async function deleteLinkLocal(id: number): Promise<void> {
  await networkDelay()
  const idx = linksStore.findIndex((l) => l.id === id)
  if (idx >= 0) linksStore.splice(idx, 1)
}
