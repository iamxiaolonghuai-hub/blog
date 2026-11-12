<script setup lang="ts">
import { onMounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'
import { blogApi } from '@/api'
import type { PostListItem, TagInfo } from '@/api/types'
import PostCard from '@/components/PostCard.vue'

const route = useRoute()
const tags = ref<TagInfo[]>([])
const posts = ref<PostListItem[]>([])
const loading = ref(true)
const activeTag = ref('')

async function load() {
  loading.value = true
  activeTag.value = (route.params.tag as string) ?? ''
  if (activeTag.value) {
    posts.value = await blogApi.listPosts(activeTag.value)
  } else {
    tags.value = await blogApi.listTags()
  }
  loading.value = false
}

function tagSize(count: number): number {
  const max = Math.max(...tags.value.map((t) => t.count), 1)
  return 13 + (count / max) * 7
}

onMounted(load)
watch(() => route.params.tag, load)
</script>

<template>
  <div class="container">
    <!-- 标签云 -->
    <template v-if="!activeTag">
      <div class="eyebrow">标签 &nbsp;·&nbsp; tags</div>
      <h1 class="page-title">标签</h1>
      <p class="page-sub">共 {{ tags.length }} 个标签，字号越大文章越多。</p>

      <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
      <div v-else class="tag-cloud">
        <router-link
          v-for="t in tags"
          :key="t.name"
          :to="`/tags/${encodeURIComponent(t.name)}`"
          class="cloud-tag"
          :style="{ fontSize: `${tagSize(t.count)}px` }"
        >
          #{{ t.name }}<span class="cloud-tag__count">{{ t.count }}</span>
        </router-link>
      </div>
    </template>

    <!-- 单个标签下的文章 -->
    <template v-else>
      <div class="eyebrow">按标签筛选 &nbsp;·&nbsp; #{{ activeTag }}</div>
      <h1 class="page-title">#{{ activeTag }}</h1>
      <p class="page-sub">该标签下共 {{ posts.length }} 篇文章。</p>

      <router-link to="/tags" class="back-link">← 返回全部标签</router-link>

      <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
      <div v-else-if="posts.length" class="post-list">
        <PostCard v-for="p in posts" :key="p.id" :post="p" />
      </div>
      <div v-else class="empty">
        <span class="empty__code">// no matches</span>
        该标签下没有文章。
      </div>
    </template>
  </div>
</template>

<style scoped>
.page-title { font-size: 32px; margin-bottom: 8px; }
.page-sub {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--ink-faint);
  margin-bottom: 30px;
}
.tag-cloud {
  display: flex;
  flex-wrap: wrap;
  gap: 12px 14px;
  padding: 12px 0;
}
.cloud-tag {
  color: var(--accent);
  font-weight: 500;
  padding: 4px 2px;
  border-bottom: 1px solid transparent;
  transition: color 0.15s ease, border-color 0.15s ease;
}
.cloud-tag:hover { color: var(--amber); border-bottom-color: var(--amber); }
.cloud-tag__count {
  font-family: var(--font-mono);
  font-size: 0.7em;
  color: var(--ink-faint);
  margin-left: 3px;
}
.back-link {
  display: inline-block;
  font-size: 13.5px;
  margin-bottom: 26px;
}
.post-list { display: flex; flex-direction: column; gap: 18px; }
</style>
