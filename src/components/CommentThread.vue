<script setup lang="ts">
import type { Comment } from '@/api/types'
import { formatDate } from '@/utils/date'
import { renderComment } from '@/utils/markdown'

defineProps<{ comment: Comment; childrenByParent: Map<number, Comment[]> }>()
const emit = defineEmits<{ reply: [comment: Comment] }>()
</script>

<template>
  <article class="thread">
    <div class="thread__head">
      <strong>{{ comment.author }}</strong>
      <time>{{ formatDate(comment.createdAt) }}</time>
      <button type="button" @click="emit('reply', comment)">回复</button>
    </div>
    <div class="markdown-body thread__body" v-html="renderComment(comment.content)"></div>
    <div v-if="childrenByParent.get(comment.id)?.length" class="thread__children">
      <CommentThread
        v-for="child in childrenByParent.get(comment.id)"
        :key="child.id"
        :comment="child"
        :children-by-parent="childrenByParent"
        @reply="emit('reply', $event)"
      />
    </div>
  </article>
</template>

<style scoped>
.thread { padding: 13px 0; border-bottom: 1px solid var(--line); }
.thread__head { display: flex; align-items: baseline; gap: 10px; font-size: 14px; }
.thread__head time { color: var(--ink-faint); font-size: 11px; }
.thread__head button { margin-left: auto; border: 0; background: none; color: var(--accent); cursor: pointer; }
.thread__body { margin-top: 6px; font-size: 14px; }
.thread__body :deep(img) { max-width: 100%; border-radius: 6px; }
.thread__children { margin: 12px 0 0 20px; padding-left: 14px; border-left: 2px solid var(--line-strong); }
@media (max-width: 600px) { .thread__children { margin-left: 8px; padding-left: 10px; } }
</style>
