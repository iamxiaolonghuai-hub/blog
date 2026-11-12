<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { adminApi } from '@/api/admin'
import { TOKEN_KEY } from '@/utils/auth'

const route = useRoute()
const router = useRouter()

const pendingCount = ref(0)
const mobileOpen = ref(false)

const menu = [
  { to: '/admin', label: '仪表盘', symbol: '◧', exact: true },
  { to: '/admin/posts', label: '文章管理', symbol: '◨', exact: false },
  { to: '/admin/comments', label: '评论管理', symbol: '◩', exact: false },
  { to: '/admin/links', label: '链接管理', symbol: '◪', exact: false },
  { to: '/admin/resources', label: '资源管理', symbol: '▣', exact: false },
  { to: '/admin/music', label: '音乐管理', symbol: '♫', exact: false },
  { to: '/admin/backups', label: '备份与恢复', symbol: '▤', exact: false },
  { to: '/admin/site', label: '站点资料', symbol: '◈', exact: false },
  { to: '/admin/password', label: '修改密码', symbol: '◇', exact: false },
]

function isActive(item: { to: string; exact: boolean }): boolean {
  return item.exact ? route.path === item.to : route.path.startsWith(item.to)
}

function logout() {
  localStorage.removeItem(TOKEN_KEY)
  router.push('/admin/login')
}

async function refreshStats() {
  const s = await adminApi.stats()
  pendingCount.value = s.pendingCommentCount
}

onMounted(refreshStats)
</script>

<template>
  <div class="admin">
    <!-- 移动端顶栏 -->
    <div class="admin__topbar">
      <button class="admin__burger" @click="mobileOpen = !mobileOpen">≡</button>
      <span class="admin__topbar-title">管理后台</span>
    </div>

    <aside class="admin__sidebar" :class="{ 'is-open': mobileOpen }">
      <div class="admin__brand">
        <span class="admin__brand-prompt">~$</span> manage
      </div>

      <nav class="admin__menu">
        <router-link
          v-for="m in menu"
          :key="m.to"
          :to="m.to"
          class="admin__link"
          :class="{ 'is-active': isActive(m) }"
          @click="mobileOpen = false"
        >
          <span class="admin__symbol">{{ m.symbol }}</span>
          <span class="admin__label">{{ m.label }}</span>
          <span v-if="m.to === '/admin/comments' && pendingCount > 0" class="admin__badge">
            {{ pendingCount }}
          </span>
        </router-link>
      </nav>

      <div class="admin__footer">
        <router-link to="/" class="admin__foot-link" target="_blank">↗ 访问前台</router-link>
        <button class="admin__foot-link" @click="logout">退出登录</button>
      </div>
    </aside>

    <div class="admin__overlay" v-if="mobileOpen" @click="mobileOpen = false"></div>

    <main class="admin__content">
      <router-view v-slot="{ Component }">
        <transition name="fade" mode="out-in">
          <component :is="Component" @data-changed="refreshStats" />
        </transition>
      </router-view>
    </main>
  </div>
</template>

<style scoped>
.admin {
  display: flex;
  min-height: 100vh;
  background: var(--bg);
}
.admin__topbar {
  display: none;
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: 52px;
  background: var(--hl-bg);
  color: #fff;
  align-items: center;
  gap: 12px;
  padding: 0 16px;
  z-index: 30;
}
.admin__burger {
  background: none;
  border: none;
  color: #fff;
  font-size: 20px;
}
.admin__topbar-title { font-family: var(--font-mono); font-size: 14px; }

.admin__sidebar {
  width: 220px;
  flex-shrink: 0;
  background: var(--hl-bg);
  color: #c3c9d2;
  display: flex;
  flex-direction: column;
  position: sticky;
  top: 0;
  height: 100vh;
}
.admin__brand {
  font-family: var(--font-mono);
  font-size: 15px;
  font-weight: 600;
  color: #fff;
  padding: 22px 20px;
  border-bottom: 1px solid #343a44;
}
.admin__brand-prompt { color: var(--hl-keyword); }

.admin__menu {
  flex: 1;
  padding: 14px 12px;
  display: flex;
  flex-direction: column;
  gap: 3px;
}
.admin__link {
  display: flex;
  align-items: center;
  gap: 11px;
  padding: 10px 12px;
  border-radius: var(--radius);
  color: #aab1bb;
  font-size: 14px;
  border-left: 2px solid transparent;
}
.admin__link:hover { background: #2a2f38; color: #fff; }
.admin__link.is-active {
  background: #2a2f38;
  color: #fff;
  border-left-color: var(--hl-keyword);
}
.admin__symbol { font-size: 14px; opacity: 0.8; }
.admin__badge {
  margin-left: auto;
  background: var(--hl-keyword);
  color: #1a1d21;
  font-size: 11px;
  font-weight: 700;
  min-width: 20px;
  height: 20px;
  padding: 0 6px;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.admin__footer {
  padding: 14px 12px;
  border-top: 1px solid #343a44;
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.admin__foot-link {
  text-align: left;
  background: none;
  border: none;
  padding: 8px 12px;
  font-family: var(--font-mono);
  font-size: 12.5px;
  color: #8b93a0;
}
.admin__foot-link:hover { color: #fff; }

.admin__content {
  flex: 1;
  min-width: 0;
  padding: 34px 40px 60px;
}
.admin__overlay { display: none; }

@media (max-width: 760px) {
  .admin__topbar { display: flex; }
  .admin__sidebar {
    position: fixed;
    left: 0;
    top: 52px;
    height: calc(100vh - 52px);
    transform: translateX(-100%);
    transition: transform 0.2s ease;
    z-index: 40;
  }
  .admin__sidebar.is-open { transform: translateX(0); }
  .admin__overlay {
    display: block;
    position: fixed;
    inset: 52px 0 0 0;
    background: rgba(0,0,0,0.4);
    z-index: 20;
  }
  .admin__content { padding: 70px 18px 40px; }
}
</style>
