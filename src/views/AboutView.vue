<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { SiteInfo } from '@/api/types'
import SiteContentSections from '@/components/SiteContentSections.vue'

const site = ref<SiteInfo | null>(null)

onMounted(async () => {
  site.value = await blogApi.getSite()
})

const defaultSkills = [
  { group: '后端', items: ['C++', 'Drogon', 'MySQL', 'Redis', 'Docker'] },
  { group: '前端', items: ['Vue 3', 'TypeScript', 'Vite', 'Pinia'] },
  { group: '其他', items: ['Linux', 'Nginx', 'JWT', '嵌入式'] },
]
</script>

<template>
  <div class="container">
    <SiteContentSections :sections="site?.contentSections" page="about" position="top" />
    <div class="eyebrow">关于 &nbsp;·&nbsp; about</div>

    <div class="about-head">
      <div class="about-avatar"><img v-if="site?.avatar" :src="site.avatar" :alt="`${site.owner}的头像`" /><span v-else>{{ site?.owner?.charAt(0) || '肖' }}</span></div>
      <div>
        <h1 class="about-name">{{ site?.owner ?? '肖龙淮' }}</h1>
        <p class="about-title">{{ site?.title ?? '系统、代码与思考' }}</p>
        <p class="about-loc" v-if="site?.location">// {{ site.location }}</p>
      </div>
    </div>

    <div class="about-body">
      <p>{{ site?.bio ?? '在这里介绍你自己。' }}</p>
    </div>

    <SiteContentSections :sections="site?.contentSections" page="about" position="middle" />

    <h2 v-if="(site?.skills ?? defaultSkills).length" class="about-h2">{{ site?.profileSectionTitle || '个人方向与兴趣' }}</h2>
    <div v-if="(site?.skills ?? defaultSkills).length" class="skills">
      <div v-for="s in site?.skills ?? defaultSkills" :key="s.group" class="skill-block">
        <p class="skill-block__name">{{ s.group }}</p>
        <div class="skill-block__items">
          <span v-for="i in s.items" :key="i" class="skill-item">{{ i }}</span>
        </div>
      </div>
    </div>

    <h2 class="about-h2">联系方式</h2>
    <div class="contact">
      <a v-if="site?.email" :href="`mailto:${site.email}`" class="contact-link">Email</a>
      <a v-if="site?.github" :href="site.github" target="_blank" rel="noopener noreferrer" class="contact-link">GitHub</a>
    </div>
    <SiteContentSections :sections="site?.contentSections" page="about" position="bottom" />
  </div>
</template>

<style scoped>
.about-head {
  display: flex;
  align-items: center;
  gap: 22px;
  margin-bottom: 36px;
}
.about-avatar {
  width: 76px;
  height: 76px;
  border-radius: var(--radius-lg);
  background: var(--accent);
  color: #fff;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 32px;
  font-weight: 600;
  font-family: var(--font-display);
  overflow: hidden;
}
.about-avatar img { display: block; width: 100%; height: 100%; object-fit: cover; }
.about-name { font-size: 30px; margin-bottom: 4px; }
.about-title {
  font-size: 15px;
  color: var(--ink-soft);
}
.about-loc {
  font-family: var(--font-mono);
  font-size: 12.5px;
  color: var(--amber);
  margin-top: 4px;
}
.about-body {
  font-family: var(--font-body);
  font-size: 17px;
  line-height: 1.8;
  color: var(--ink-soft);
  margin-bottom: 36px;
}
.about-body p { margin-bottom: 16px; }
.about-h2 {
  font-size: 20px;
  margin: 32px 0 16px;
}
.about-h2::before {
  content: '##';
  font-family: var(--font-mono);
  color: var(--amber);
  font-size: 0.75em;
  margin-right: 8px;
  font-weight: 400;
}
.skills {
  display: flex;
  flex-direction: column;
  gap: 14px;
}
.skill-block {
  display: flex;
  align-items: baseline;
  gap: 16px;
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius);
  padding: 14px 18px;
}
.skill-block__name {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--amber);
  min-width: 44px;
  font-weight: 600;
}
.skill-block__items { display: flex; flex-wrap: wrap; gap: 8px; }
.skill-item {
  font-size: 13.5px;
  color: var(--accent);
  background: var(--accent-soft);
  padding: 3px 11px;
  border-radius: 3px;
}
.contact { display: flex; gap: 12px; }
.contact-link {
  padding: 9px 20px;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  font-size: 14px;
  color: var(--ink);
}
.contact-link:hover { border-color: var(--accent); color: var(--accent); }
</style>
