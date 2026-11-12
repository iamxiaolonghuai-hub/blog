<script setup lang="ts">
import { ref } from 'vue'
import { useRoute } from 'vue-router'

const open = ref(false)
const route = useRoute()

const links = [
  { to: '/', label: '首页' },
  { to: '/posts', label: '文章' },
  { to: '/tags', label: '标签' },
  { to: '/archive', label: '归档' },
  { to: '/links', label: '友链' },
  { to: '/resources', label: '资源' },
  { to: '/about', label: '关于' },
]

function isActive(to: string): boolean {
  return to === '/' ? route.path === '/' : route.path.startsWith(to)
}

function close() {
  open.value = false
}
</script>

<template>
  <header class="site-nav">
    <div class="site-nav__inner">
      <router-link to="/" class="brand" @click="close">
        <span class="brand__prompt">~$</span>&nbsp;notes<span class="brand__cursor"></span>
      </router-link>

      <button class="nav-toggle" aria-label="切换菜单" @click="open = !open">
        {{ open ? '×' : '≡' }}
      </button>

      <nav class="nav-links" :class="{ 'is-open': open }">
        <router-link
          v-for="l in links"
          :key="l.to"
          :to="l.to"
          custom
          v-slot="{ navigate }"
        >
          <a
            :class="{ 'router-link-active': isActive(l.to) }"
            @click="navigate(); close()"
            >{{ l.label }}</a
          >
        </router-link>
      </nav>
    </div>
  </header>
</template>
