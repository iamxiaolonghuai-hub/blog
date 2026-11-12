<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { ResourceItem } from '@/api/types'

const resources = ref<ResourceItem[]>([])
const loading = ref(true)
const error = ref('')
onMounted(async () => {
  try { resources.value = await blogApi.listResources() }
  catch { error.value = '资源加载失败，请稍后刷新' }
  finally { loading.value = false }
})
</script>

<template>
  <main class="container container--wide resources">
    <header class="resources__head">
      <div class="eyebrow">资源 · resources</div>
      <h1>资源分享</h1>
      <p>浏览资料介绍，进入详情后选择下载方式。复制链接并跳转时需要登录读者账号。</p>
    </header>
    <p v-if="loading" class="state">加载中…</p>
    <p v-else-if="error" class="state" role="alert">{{ error }}</p>
    <p v-else-if="!resources.length" class="state">资源正在整理中，稍后再来看看。</p>
    <div v-else class="resource-grid">
      <router-link v-for="item in resources" :key="item.id" :to="`/resources/${item.id}`" class="resource-card">
        <div class="resource-card__image">
          <img v-if="item.image" :src="item.image" :alt="item.title" loading="lazy" />
          <span v-else>~/resource</span>
        </div>
        <div class="resource-card__body">
          <div class="resource-card__meta">RESOURCE <span>{{ String(item.id).padStart(2, '0') }}</span></div>
          <h2>{{ item.title }}</h2>
          <p class="description">{{ item.description }}</p>
          <div class="platforms"><span v-for="label in item.linkLabels || []" :key="label">{{ label }}</span></div>
          <span class="detail-link">查看资源详情 ↗</span>
        </div>
      </router-link>
    </div>
  </main>
</template>

<style scoped>
.resources { padding-top: 50px; padding-bottom: 76px; }
.resources__head { margin-bottom: 30px; }
.resources__head h1 { margin: 10px 0 12px; font-size: 34px; }
.resources__head > p { max-width: 640px; color: var(--ink-soft); font-size: 14px; }
.state { padding: 34px 0; color: var(--ink-soft); }
.resource-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 18px; }
.resource-card { display: block; overflow: hidden; color: inherit; border: 1px solid var(--line); border-radius: var(--radius-lg); background: var(--bg-raised); transition: transform .18s ease, box-shadow .18s ease; }
.resource-card:hover { transform: translateY(-2px); box-shadow: var(--shadow-card); color: inherit; }
.resource-card__image { display: flex; align-items: center; justify-content: center; height: 180px; background: var(--bg-inset); color: var(--amber); font-family: var(--font-mono); font-size: 14px; }
.resource-card__image img { width: 100%; height: 100%; object-fit: cover; }
.resource-card__body { padding: 20px; }
.resource-card__meta { display: flex; justify-content: space-between; color: var(--amber); font: 11px var(--font-mono); letter-spacing: .08em; margin-bottom: 8px; }
.resource-card h2 { font-size: 21px; }
.description { display: -webkit-box; -webkit-box-orient: vertical; -webkit-line-clamp: 3; overflow: hidden; white-space: pre-wrap; color: var(--ink-soft); font: 15px/1.65 var(--font-body); margin: 12px 0 18px; }
.platforms { display: flex; flex-wrap: wrap; gap: 7px; margin-bottom: 16px; }
.platforms span { padding: 4px 8px; border-radius: var(--radius); background: var(--bg-inset); color: var(--ink-soft); font-size: 12px; }
.detail-link { color: var(--accent); font-size: 13px; }
@media(max-width:650px) { .resource-grid { grid-template-columns: 1fr; } .resources__head h1 { font-size: 28px; } }
</style>
