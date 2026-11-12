<script setup lang="ts">
import { onMounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'
import { blogApi } from '@/api'
import type { Post, TocItem } from '@/api/types'
import { renderMarkdown } from '@/utils/markdown'
import { formatDate } from '@/utils/date'
import Toc from '@/components/Toc.vue'
import PostMeta from '@/components/PostMeta.vue'
import CommentSection from '@/components/CommentSection.vue'
import { selectArticleMusic } from '@/utils/musicSelection'

const route = useRoute()
const post = ref<Post | null>(null)
const headings = ref<TocItem[]>([])
const html = ref('')
const loading = ref(true)
const notFound = ref(false)

async function load() {
  loading.value = true
  notFound.value = false
  const slug = route.params.slug as string
  try {
    const p = await blogApi.getPost(slug)
    post.value = p
    selectArticleMusic(p.musicId)
    const result = renderMarkdown(p.content)
    html.value = result.html
    headings.value = result.headings
  } catch {
    notFound.value = true
  }
  loading.value = false
  window.scrollTo({ top: 0 })
}

onMounted(load)
watch(() => route.params.slug, load)
</script>

<template>
  <div class="container container--wide">
    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>

    <div v-else-if="notFound" class="empty">
      <span class="empty__code">// cat: no such file</span>
      文章不存在或已被删除。
      <router-link to="/posts" class="notfound-link">← 返回文章列表</router-link>
    </div>

    <article v-else-if="post" class="post-layout">
      <div class="post-main">
        <p class="post-path"><span>$</span> cat ~/posts/{{ post.slug }}.md</p>

        <h1 class="post-title">{{ post.title }}</h1>

        <div class="post-head">
          <PostMeta :date="post.createdAt" :minutes="post.readingMinutes" :views="post.views" />
          <span v-if="post.category" class="post-category">{{ post.category }}</span>
        </div>

        <div class="post-tags">
          <router-link v-for="t in post.tags" :key="t" :to="`/tags/${encodeURIComponent(t)}`" class="tag">
            #{{ t }}
          </router-link>
        </div>

        <div class="markdown-body" v-html="html"></div>

        <div class="post-foot">
          <router-link to="/posts" class="back-link">← 返回文章列表</router-link>
          <p class="post-updated">最后更新于 {{ formatDate(post.updatedAt) }}</p>
        </div>

        <CommentSection :slug="post.slug" />
      </div>

      <aside v-if="headings.length" class="post-aside">
        <Toc :items="headings" />
      </aside>
    </article>
  </div>
</template>

<style scoped>
.post-layout {
  display: grid;
  grid-template-columns: 1fr 210px;
  gap: 44px;
  align-items: start;
}
.post-path {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--ink-faint);
  margin-bottom: 16px;
}
.post-path span { color: var(--amber); }
.post-title {
  font-size: 34px;
  line-height: 1.25;
  margin-bottom: 16px;
  letter-spacing: -0.01em;
}
.post-head {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding-bottom: 16px;
  border-bottom: 1px solid var(--line);
  margin-bottom: 16px;
}
.post-category {
  font-family: var(--font-mono);
  font-size: 12px;
  color: var(--amber);
  background: var(--amber-soft);
  padding: 3px 10px;
  border-radius: 3px;
}
.post-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 7px;
  margin-bottom: 34px;
}
.post-foot {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  margin-top: 48px;
  padding-top: 20px;
  border-top: 1px solid var(--line);
}
.back-link { font-size: 14px; font-weight: 500; }
.post-updated {
  font-family: var(--font-mono);
  font-size: 12px;
  color: var(--ink-faint);
}
.notfound-link { display: block; margin-top: 16px; }

@media (max-width: 860px) {
  .post-layout { grid-template-columns: 1fr; gap: 0; }
  .post-aside { display: none; }
}
</style>
