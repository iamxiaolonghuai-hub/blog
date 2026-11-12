<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { PostListItem } from '@/api/types'
import PostCard from '@/components/PostCard.vue'

const posts = ref<PostListItem[]>([])
const loading = ref(true)

onMounted(async () => {
  posts.value = await blogApi.listPosts()
  loading.value = false
})
</script>

<template>
  <div class="container">
    <div class="eyebrow">全部文章 &nbsp;·&nbsp; all posts</div>
    <h1 class="page-title">文章</h1>
    <p class="page-sub">共 {{ posts.length }} 篇，置顶与展示权重优先。</p>

    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>

    <div v-else-if="posts.length" class="post-list">
      <PostCard v-for="p in posts" :key="p.id" :post="p" />
    </div>

    <div v-else class="empty">
      <span class="empty__code">// 404 no posts</span>
      还没有文章。
    </div>
  </div>
</template>

<style scoped>
.page-title {
  font-size: 32px;
  margin-bottom: 8px;
}
.page-sub {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--ink-faint);
  margin-bottom: 32px;
}
.post-list {
  display: flex;
  flex-direction: column;
  gap: 18px;
}
</style>
