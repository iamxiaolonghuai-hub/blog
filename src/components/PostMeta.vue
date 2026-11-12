<script setup lang="ts">
import { formatDate } from '@/utils/date'

defineProps<{
  date: string
  minutes: number
  views?: number
}>()
</script>

<template>
  <div class="post-meta">
    <span class="post-meta__item">{{ formatDate(date) }}</span>
    <span class="post-meta__sep">/</span>
    <span class="post-meta__item">{{ minutes }} min read</span>
    <template v-if="views !== undefined">
      <span class="post-meta__sep">/</span>
      <span class="post-meta__item">{{ views }} views</span>
    </template>
  </div>
</template>

<style scoped>
.post-meta {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 7px;
  font-family: var(--font-mono);
  font-size: 12.5px;
  color: var(--ink-faint);
}
.post-meta__sep {
  color: var(--amber);
}
</style>
