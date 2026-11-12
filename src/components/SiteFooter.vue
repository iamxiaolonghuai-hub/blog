<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { FriendLink, SiteInfo } from '@/api/types'
const year = new Date().getFullYear()
const links = ref<FriendLink[]>([])
const site = ref<SiteInfo | null>(null)
onMounted(async () => {
  const [siteResult, linkResult] = await Promise.allSettled([blogApi.getSite(), blogApi.listLinks()])
  if (siteResult.status === 'fulfilled') site.value = siteResult.value
  if (linkResult.status === 'fulfilled') links.value = linkResult.value.filter(link => link.visible && /^https?:\/\//i.test(link.url))
})
</script>

<template>
  <footer class="site-footer">
    <div class="site-footer__inner">
      <span>© {{ year }} {{ site?.name ?? '龙淮的札记' }} · Built with Vue &amp; Drogon</span>
      <div class="footer-links">
        <a v-if="site?.github" :href="site.github" target="_blank" rel="noopener noreferrer">GitHub</a>
        <router-link to="/about">关于</router-link>
        <a v-if="site?.email" :href="`mailto:${site.email}`">Email</a>
      </div>
    </div>
    <div v-if="links.length" class="site-footer__friends">
      <span>友情链接</span>
      <a v-for="link in links" :key="link.id" :href="link.url" target="_blank" rel="noopener noreferrer">{{ link.name }}</a>
    </div>
  </footer>
</template>

<style scoped>
.site-footer__friends { max-width: 1200px; margin: 0 auto; padding: 12px 24px 20px; display: flex; flex-wrap: wrap; gap: 10px 20px; font-size: 13px; }
.site-footer__friends span { color: var(--ink-faint); }
</style>
