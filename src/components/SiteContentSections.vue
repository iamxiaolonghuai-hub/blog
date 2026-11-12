<script setup lang="ts">
import { computed } from 'vue'
import type { SiteContentSection } from '@/api/types'
import { renderSafeHtml } from '@/utils/safeHtml'

const props = defineProps<{ sections?: SiteContentSection[]; page: 'home' | 'about'; position: 'top' | 'middle' | 'bottom' }>()
const visible = computed(() => (props.sections ?? []).filter(section => section.page === props.page && (section.slot || 'middle') === props.position && section.visible))
</script>

<template>
  <section v-for="section in visible" :key="section.id" class="content-section">
    <h2 v-if="section.title">{{ section.title }}</h2>
    <div class="markdown-body content-section__body" v-html="renderSafeHtml(section.html)"></div>
  </section>
</template>

<style scoped>
.content-section { margin: 28px 0 48px; padding: 24px; border: 1px solid var(--line); border-radius: var(--radius-lg); background: var(--bg-raised); }
.content-section h2 { font-size: 23px; margin-bottom: 16px; }
.content-section__body { overflow-wrap: anywhere; line-height: 1.75; }
.content-section__body :deep(img) { max-width: 100%; height: auto; border-radius: var(--radius); }
.content-section__body :deep(table) { display: block; max-width: 100%; overflow-x: auto; }
</style>
