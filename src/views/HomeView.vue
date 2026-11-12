<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { PostListItem, SiteInfo } from '@/api/types'
import PostCard from '@/components/PostCard.vue'

const site = ref<SiteInfo | null>(null)
const posts = ref<PostListItem[]>([])
const loading = ref(true)
const loadError = ref('')

onMounted(async () => {
  try {
  const [s, p] = await Promise.all([blogApi.getSite(), blogApi.listPosts()])
  site.value = s
  posts.value = p.slice(0, 5)
  } catch { loadError.value = '文章加载失败，请稍后刷新重试' }
  finally { loading.value = false }
})
</script>

<template>
  <div class="container">
    <!-- Hero -->
    <section class="hero">
      <div class="hero__text">
        <p class="hero__kicker"><span>{ }</span> {{ site?.homeKicker ?? '个人博客 · personal blog' }}</p>
        <h1 class="hero__title">{{ site?.homeHeroTitle ?? '在终端与代码之间，\n记录构建系统的过程。' }}</h1>
        <p class="hero__bio">
          {{ site?.bio ?? '一个系统程序员，写 C++ 后端，也写 Vue 前端。' }}
        </p>
        <div class="hero__actions">
          <router-link to="/posts" class="btn btn--primary">阅读文章</router-link>
          <router-link to="/about" class="btn btn--ghost">关于我</router-link>
        </div>
      </div>

      <div class="terminal hero__terminal">
        <div class="terminal__bar">
          <span class="terminal__dot terminal__dot--r"></span>
          <span class="terminal__dot terminal__dot--y"></span>
          <span class="terminal__dot terminal__dot--g"></span>
          <span class="terminal__title">{{ site?.homeTerminalTitle ?? 'zsh — xiaolonghuai' }}</span>
        </div>
        <div class="terminal__body">
          <div><span class="terminal__prompt">$</span> whoami</div>
          <div class="terminal__out">{{ site?.homeIntro ?? '肖龙淮 · C++ backend / Vue frontend' }}</div>
          <div><span class="terminal__prompt">$</span> ls ~/posts</div>
          <div v-for="post in posts.slice(0, 3)" :key="post.id" class="terminal__amber">{{ post.slug }}.md</div>
          <div v-if="!posts.length" class="terminal__amber">暂无已发布文章</div>
          <div><span class="terminal__prompt">$</span> cat motto.txt</div>
          <div class="terminal__ok">// {{ site?.homeMotto ?? '把想清楚的问题，写下来。' }}</div>
          <div><span class="terminal__prompt">$</span> <span class="terminal__cursor">▋</span></div>
        </div>
      </div>
    </section>


    <!-- 最新文章 -->
    <section class="latest">
      <div class="latest__head"><div><div class="eyebrow">最新文章 &nbsp;·&nbsp; latest</div><h2>最近写下的内容</h2></div><router-link to="/posts">全部文章 ↗</router-link></div>

      <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
      <p v-else-if="loadError" role="alert">{{ loadError }}</p>
      <p v-else-if="!posts.length" class="empty">暂无已发布文章。你可以在后台将草稿发布。</p>

      <div v-else class="post-list">
        <PostCard v-for="(p, index) in posts" :key="p.id" :post="p" :featured="index === 0 && !!p.cover" />
      </div>

      <div class="latest__more">
        <router-link to="/posts">查看全部文章 →</router-link>
      </div>
    </section>
  </div>
</template>

<style scoped>
.hero {
  display: grid;
  grid-template-columns: 1.1fr 0.9fr;
  gap: 40px;
  align-items: center;
  padding: 24px 0 64px;
}
.hero__kicker {
  font-family: var(--font-mono);
  font-size: 12.5px;
  color: var(--amber);
  margin-bottom: 18px;
}
.hero__title {
  white-space: pre-line;
  font-size: 38px;
  line-height: 1.2;
  letter-spacing: -0.01em;
  margin-bottom: 20px;
}
.hero__bio {
  font-family: var(--font-body);
  font-size: 17px;
  line-height: 1.7;
  color: var(--ink-soft);
  margin-bottom: 28px;
  max-width: 440px;
}
.hero__actions {
  display: flex;
  gap: 12px;
}

.btn {
  display: inline-flex;
  align-items: center;
  padding: 10px 20px;
  border-radius: var(--radius);
  font-size: 14.5px;
  font-weight: 500;
  border: 1px solid transparent;
  transition: all 0.16s ease;
}
.btn--primary {
  background: var(--accent);
  color: #fff;
}
.btn--primary:hover { background: var(--accent-hover); color: #fff; }
.btn--ghost {
  border-color: var(--line-strong);
  color: var(--ink);
}
.btn--ghost:hover { border-color: var(--accent); color: var(--accent); }

.latest { padding-top: 8px; }
.latest__head { display: flex; align-items: end; justify-content: space-between; gap: 16px; margin-bottom: 20px; }
.latest__head h2 { margin-top: 7px; font-size: 23px; }
.latest__head a { font-size: 13px; white-space: nowrap; }
.post-list {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}
.post-list :deep(.post-card--featured) { grid-column: 1 / -1; }
.latest__more {
  margin-top: 26px;
  font-size: 14.5px;
  font-weight: 500;
}

.terminal__cursor {
  animation: blink 1.1s steps(1) infinite;
  color: var(--hl-string);
}
@keyframes blink { 50% { opacity: 0; } }

@media (max-width: 820px) {
  .hero {
    grid-template-columns: 1fr;
    gap: 32px;
  }
  .hero__title { font-size: 31px; }
}
@media (max-width: 640px) { .post-list { grid-template-columns: 1fr; } }
</style>
