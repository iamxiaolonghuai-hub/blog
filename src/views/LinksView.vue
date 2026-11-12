<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { FriendLink } from '@/api/types'

const links = ref<FriendLink[]>([])
const loading = ref(true)
const error = ref('')

onMounted(async () => {
  try {
    links.value = await blogApi.listLinks()
  } catch {
    error.value = '友链加载失败，请稍后重试'
  } finally {
    loading.value = false
  }
})
</script>

<template>
  <div class="container">
    <div class="eyebrow">友情链接 &nbsp;·&nbsp; links</div>
    <h1 class="page-title">友链</h1>
    <p class="page-sub">常逛的站点，与优秀的人互相照亮。</p>

    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
    <div v-else-if="error" class="empty">{{ error }}</div>

    <div v-else-if="links.length" class="link-grid">
      <a
        v-for="l in links"
        :key="l.id"
        class="link-card"
        :href="l.url"
        target="_blank"
        rel="noopener noreferrer"
      >
        <img v-if="l.logo" class="link-card__logo" :src="l.logo" :alt="l.name" />
        <span v-else class="link-card__logo link-card__logo--fallback">
          {{ l.name.slice(0, 1) }}
        </span>
        <span class="link-card__meta">
          <span class="link-card__name">{{ l.name }}</span>
          <span v-if="l.description" class="link-card__desc">{{ l.description }}</span>
        </span>
      </a>
    </div>

    <div v-else class="empty">
      <span class="empty__code">// no links yet</span>
      还没有友链，欢迎交换。
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
.link-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
  gap: 14px;
}
.link-card {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 14px 16px;
  border: 1px solid var(--line);
  border-radius: var(--radius);
  background: var(--bg-raised);
  text-decoration: none;
  transition: border-color 0.15s ease, transform 0.15s ease;
}
.link-card:hover {
  border-color: var(--accent);
  transform: translateY(-2px);
}
.link-card__logo {
  width: 40px;
  height: 40px;
  border-radius: 8px;
  object-fit: cover;
  flex-shrink: 0;
}
.link-card__logo--fallback {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background: var(--accent-soft);
  color: var(--accent);
  font-weight: 700;
  font-size: 16px;
}
.link-card__meta {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
}
.link-card__name {
  font-weight: 600;
  font-size: 14.5px;
  color: var(--ink);
}
.link-card__desc {
  font-size: 12.5px;
  color: var(--ink-faint);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
</style>
