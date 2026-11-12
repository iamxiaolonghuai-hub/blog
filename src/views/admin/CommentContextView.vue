<script setup lang="ts">
import { computed, nextTick, onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import { adminApi } from '@/api/admin'
import type { Comment, Post } from '@/api/types'
import { formatDate } from '@/utils/date'
import { renderComment, renderMarkdown } from '@/utils/markdown'

const route = useRoute()
const targetId = Number(route.params.id)
const post = ref<Post | null>(null)
const comments = ref<Comment[]>([])
const articleHtml = ref('')
const loading = ref(true)
const error = ref('')
const target = computed(() => comments.value.find(comment => comment.id === targetId))
const byId = computed(() => new Map(comments.value.map(comment => [comment.id, comment])))
const ordered = computed(() => {
  const result: { comment: Comment; depth: number }[] = []
  const seen = new Set<number>()
  const children = new Map<number, Comment[]>()
  const roots: Comment[] = []
  const sorted = [...comments.value].sort((a, b) => a.createdAt.localeCompare(b.createdAt) || a.id - b.id)
  for (const comment of sorted) {
    if (comment.parentId != null && byId.value.has(comment.parentId)) {
      const group = children.get(comment.parentId) ?? []
      group.push(comment)
      children.set(comment.parentId, group)
    } else roots.push(comment)
  }
  function visit(comment: Comment, depth: number) {
    if (seen.has(comment.id)) return
    seen.add(comment.id)
    result.push({ comment, depth })
    for (const child of children.get(comment.id) ?? []) visit(child, depth + 1)
  }
  for (const root of roots) visit(root, 0)
  for (const comment of sorted) visit(comment, 0)
  return result
})

async function load() {
  loading.value = true
  error.value = ''
  try {
    const all = await adminApi.listComments()
    const selected = all.find(comment => comment.id === targetId)
    if (!selected) { error.value = '这条评论不存在或已被删除'; return }
    const article = await adminApi.getPost(selected.postId)
    post.value = article
    articleHtml.value = renderMarkdown(article.content).html
    comments.value = all.filter(comment => comment.postId === selected.postId)
    loading.value = false
    await nextTick()
    requestAnimationFrame(() => document.getElementById(`admin-comment-${targetId}`)?.scrollIntoView({ block: 'center' }))
  } catch { error.value = '文章或评论加载失败，请稍后重试' }
  finally { loading.value = false }
}
onMounted(load)
</script>

<template>
  <section class="context">
    <router-link to="/admin/comments" class="back">← 返回评论管理</router-link>
    <p v-if="loading">加载中…</p>
    <p v-else-if="error" role="alert">{{ error }}</p>
    <template v-else-if="post && target">
      <header>
        <div class="eyebrow">评论上下文</div>
        <h1>{{ post.title }}</h1>
        <p>正在查看 {{ target.author }} 的评论 · {{ formatDate(target.createdAt) }}</p>
        <router-link :to="`/posts/${encodeURIComponent(post.slug)}`" target="_blank">在前台打开文章 ↗</router-link>
      </header>
      <details class="article-preview">
        <summary>展开文章正文</summary>
        <div class="markdown-body" v-html="articleHtml"></div>
      </details>
      <h2>这篇文章的评论 <small>{{ comments.length }}</small></h2>
      <p class="hint">包含待审核和已拒绝的评论，仅管理员可见。选中的评论已高亮。</p>
      <div class="thread">
        <article v-for="entry in ordered" :id="`admin-comment-${entry.comment.id}`" :key="entry.comment.id" class="comment" :class="{ selected: entry.comment.id === targetId }" :style="{ marginLeft: `${Math.min(entry.depth, 4) * 18}px` }">
          <div class="comment-head">
            <strong>{{ entry.comment.author }}</strong>
            <span v-if="entry.comment.parentId != null">回复 {{ byId.get(entry.comment.parentId)?.author || '已删除的评论' }}</span>
            <time>{{ formatDate(entry.comment.createdAt) }}</time>
            <span class="status">{{ entry.comment.status === 'approved' ? '已通过' : entry.comment.status === 'pending' ? '待审核' : '已拒绝' }}</span>
          </div>
          <div class="markdown-body content" v-html="renderComment(entry.comment.content)"></div>
        </article>
      </div>
    </template>
  </section>
</template>

<style scoped>
.context { max-width: 950px; }
.back { font-size: 13px; }
header { margin: 24px 0; }
h1 { margin: 7px 0; font-size: 28px; }
header p, .hint { color: var(--ink-soft); font-size: 13px; }
header a { display: inline-block; margin-top: 10px; }
.article-preview { margin: 22px 0 30px; padding: 16px 20px; border: 1px solid var(--line); border-radius: var(--radius-lg); background: var(--bg-raised); }
summary { cursor: pointer; color: var(--accent); }
.article-preview .markdown-body { margin-top: 22px; }
h2 { font-size: 20px; }
h2 small { color: var(--ink-faint); font-size: 13px; }
.thread { display: grid; gap: 10px; margin-top: 16px; }
.comment { padding: 14px 16px; border: 1px solid var(--line); border-left: 3px solid var(--line-strong); border-radius: var(--radius); background: var(--bg-raised); scroll-margin-top: 80px; }
.comment.selected { border-color: var(--accent); border-left-width: 5px; background: var(--accent-soft); box-shadow: var(--shadow-card); }
.comment-head { display: flex; flex-wrap: wrap; align-items: baseline; gap: 8px; font-size: 13px; }
.comment-head span, time { color: var(--ink-faint); }
.comment-head .status { margin-left: auto; }
.content { margin-top: 9px; font-size: 14px; overflow-wrap: anywhere; }
.content :deep(img) { max-width: 100%; border-radius: var(--radius); }
[role=alert] { margin-top: 20px; color: #b3351f; }
</style>
