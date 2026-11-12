<script setup lang="ts">
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import NavBar from '@/components/NavBar.vue'
import SiteFooter from '@/components/SiteFooter.vue'
import MusicPlayer from '@/components/MusicPlayer.vue'

const route = useRoute()
const isAdmin = computed(() => route.path.startsWith('/admin'))
</script>

<template>
  <NavBar v-if="!isAdmin" />
  <main :class="{ 'main--admin': isAdmin }">
    <router-view v-slot="{ Component }">
      <transition name="fade" mode="out-in">
        <component :is="Component" />
      </transition>
    </router-view>
  </main>
  <SiteFooter v-if="!isAdmin" />
  <MusicPlayer v-if="!isAdmin" />
</template>

<style scoped>
.main--admin {
  padding: 0;
}
</style>
