<script setup lang="ts">
import { onBeforeUnmount, onMounted, ref } from 'vue'
import type { TocItem } from '@/api/types'

const props = defineProps<{ items: TocItem[] }>()

const activeId = ref('')

function onScroll() {
  let current = props.items[0]?.id ?? ''
  for (const h of props.items) {
    const el = document.getElementById(h.id)
    if (el && el.getBoundingClientRect().top <= 110) {
      current = h.id
    }
  }
  activeId.value = current
}

function scrollTo(id: string) {
  const el = document.getElementById(id)
  if (!el) return
  const top = el.getBoundingClientRect().top + window.scrollY - 76
  window.scrollTo({ top, behavior: 'smooth' })
}

onMounted(() => {
  window.addEventListener('scroll', onScroll, { passive: true })
  onScroll()
})
onBeforeUnmount(() => window.removeEventListener('scroll', onScroll))
</script>

<template>
  <nav class="toc" aria-label="文章目录">
    <p class="toc__title">// contents</p>
    <ul>
      <li v-for="h in items" :key="h.id">
        <button
          class="toc__link"
          :class="{
            'is-active': h.id === activeId,
            'toc__link--sub': h.level === 3,
          }"
          @click="scrollTo(h.id)"
        >
          {{ h.text }}
        </button>
      </li>
    </ul>
  </nav>
</template>

<style scoped>
.toc {
  position: sticky;
  top: 92px;
  max-height: calc(100vh - 120px);
  overflow-y: auto;
}
.toc__title {
  font-family: var(--font-mono);
  font-size: 12px;
  color: var(--ink-faint);
  margin: 0 0 12px;
}
.toc ul {
  list-style: none;
  margin: 0;
  padding: 0;
  border-left: 1px solid var(--line);
}
.toc__link {
  display: block;
  width: 100%;
  text-align: left;
  background: none;
  border: none;
  padding: 6px 0 6px 14px;
  font-family: var(--font-display);
  font-size: 13.5px;
  line-height: 1.4;
  color: var(--ink-faint);
  border-left: 2px solid transparent;
  margin-left: -1px;
  transition: color 0.15s ease, border-color 0.15s ease;
}
.toc__link--sub {
  padding-left: 26px;
  font-size: 13px;
}
.toc__link:hover { color: var(--ink); }
.toc__link.is-active {
  color: var(--accent);
  border-left-color: var(--amber);
  font-weight: 600;
}
</style>
