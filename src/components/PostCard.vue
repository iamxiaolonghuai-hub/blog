<script setup lang="ts">
import type { PostListItem } from '@/api/types'
import PostMeta from './PostMeta.vue'

defineProps<{ post: PostListItem; featured?: boolean }>()
</script>

<template>
  <article class="post-card" :class="{ 'post-card--featured': featured }">
    <router-link v-if="post.cover" :to="`/posts/${post.slug}`" class="post-card__cover">
      <img :src="post.cover" :alt="post.title" loading="lazy" />
    </router-link>
    <div class="post-card__content">
    <div class="post-card__labels"><span v-if="post.category" class="post-card__category">{{ post.category }}</span><span v-if="post.isPinned" class="post-card__badge">置顶</span><span v-if="post.isHot" class="post-card__badge post-card__badge--hot">热门</span></div>
    <router-link :to="`/posts/${post.slug}`" class="post-card__path">
      <span class="post-card__prompt">$</span> cat ~/posts/{{ post.slug }}.md
    </router-link>

    <h2 class="post-card__title">
      <router-link :to="`/posts/${post.slug}`">{{ post.title }}</router-link>
    </h2>

    <p class="post-card__summary">{{ post.summary }}</p>

    <div class="post-card__foot">
      <PostMeta :date="post.createdAt" :minutes="post.readingMinutes" :views="post.views" />
      <div class="post-card__tags">
        <router-link v-for="t in post.tags" :key="t" :to="`/tags/${encodeURIComponent(t)}`" class="tag">
          #{{ t }}
        </router-link>
      </div>
    </div>
    </div>
  </article>
</template>

<style scoped>
.post-card {
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  padding: 22px 24px;
  transition: transform 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
}
.post-card--featured { display: grid; grid-template-columns: minmax(0, 1fr) minmax(0, 1.35fr); gap: 0; padding: 0; overflow: hidden; }
.post-card--featured .post-card__content { padding: 24px; }
.post-card__cover { display: block; overflow: hidden; max-height: 260px; }
.post-card__cover img { width: 100%; height: 100%; min-height: 180px; object-fit: cover; transition: transform .25s ease; }
.post-card:hover .post-card__cover img { transform: scale(1.03); }
.post-card__category { display: inline-block; margin-bottom: 8px; color: var(--amber); font-family: var(--font-mono); font-size: 11px; }
.post-card__labels { display: flex; align-items: center; gap: 8px; margin-bottom: 8px; }
.post-card__labels .post-card__category { margin-bottom: 0; }
.post-card__badge { padding: 2px 6px; border-radius: 3px; background: var(--accent-soft); color: var(--accent); font-size: 11px; }
.post-card__badge--hot { background: #f8e7d8; color: #a85820; }
.post-card:hover {
  transform: translateY(-2px);
  border-color: var(--line-strong);
  box-shadow: var(--shadow-card);
}

.post-card__path {
  display: inline-block;
  font-family: var(--font-mono);
  font-size: 12px;
  color: var(--ink-faint);
  margin-bottom: 10px;
}
.post-card__path:hover { color: var(--amber); }
.post-card__prompt { color: var(--amber); }

.post-card__title {
  font-size: 21px;
  margin-bottom: 9px;
}
.post-card__title a {
  color: var(--ink);
}
.post-card__title a:hover { color: var(--accent); }

.post-card__summary {
  font-family: var(--font-body);
  font-size: 15.5px;
  line-height: 1.7;
  color: var(--ink-soft);
  margin-bottom: 16px;
}
.post-card__summary { display: -webkit-box; -webkit-line-clamp: 3; line-clamp: 3; -webkit-box-orient: vertical; overflow: hidden; }

.post-card__foot {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.post-card__tags {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}
@media (max-width: 640px) { .post-card--featured { grid-template-columns: 1fr; } }
</style>
