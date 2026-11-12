<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { PostListItem } from '@/api/types'
import { formatDate, yearOf } from '@/utils/date'

const posts = ref<PostListItem[]>([])
const loading = ref(true)

const groups = computed(() => {
  const map = new Map<number, PostListItem[]>()
  ;[...posts.value]
    .sort((a, b) => b.createdAt.localeCompare(a.createdAt))
    .forEach((p) => {
      const y = yearOf(p.createdAt)
      if (!map.has(y)) map.set(y, [])
      map.get(y)!.push(p)
    })
  return [...map.entries()].map(([year, list]) => ({ year, list }))
})

onMounted(async () => {
  posts.value = await blogApi.listPosts()
  loading.value = false
})
</script>

<template>
  <div class="container">
    <div class="eyebrow">归档 &nbsp;·&nbsp; archive</div>
    <h1 class="page-title">归档</h1>
    <p class="page-sub">共 {{ posts.length }} 篇，按年份整理。</p>

    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>

    <div v-else class="timeline">
      <section v-for="g in groups" :key="g.year" class="timeline__group">
        <h2 class="timeline__year">{{ g.year }}<span class="timeline__count">{{ g.list.length }}</span></h2>
        <ul class="timeline__list">
          <li v-for="p in g.list" :key="p.id" class="timeline__item">
            <span class="timeline__date">{{ formatDate(p.createdAt) }}</span>
            <router-link :to="`/posts/${p.slug}`" class="timeline__link">{{ p.title }}</router-link>
          </li>
        </ul>
      </section>
    </div>
  </div>
</template>

<style scoped>
.page-title { font-size: 32px; margin-bottom: 8px; }
.page-sub {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--ink-faint);
  margin-bottom: 34px;
}
.timeline__group { margin-bottom: 38px; }
.timeline__year {
  display: flex;
  align-items: baseline;
  gap: 10px;
  font-size: 24px;
  color: var(--accent);
  padding-bottom: 10px;
  border-bottom: 1px solid var(--line);
  margin-bottom: 8px;
}
.timeline__count {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--ink-faint);
  font-weight: 400;
}
.timeline__list {
  list-style: none;
  margin: 0;
  padding: 0;
}
.timeline__item {
  display: flex;
  gap: 18px;
  padding: 9px 0;
  border-bottom: 1px dashed var(--line);
}
.timeline__date {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--ink-faint);
  white-space: nowrap;
  padding-top: 1px;
}
.timeline__link {
  color: var(--ink);
  font-size: 15.5px;
}
.timeline__link:hover { color: var(--amber); }
</style>
