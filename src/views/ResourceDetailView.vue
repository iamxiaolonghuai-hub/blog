<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import { blogApi } from '@/api'
import type { ResourceItem } from '@/api/types'
import { clearReaderSession, getReaderName, getReaderToken, setReaderSession } from '@/utils/auth'

const route = useRoute()
const item = ref<ResourceItem | null>(null)
const loading = ref(true)
const error = ref('')
const notice = ref('')
const readerName = ref(getReaderToken() ? getReaderName() : null)
const showAuth = ref(false)
const accountMode = ref<'login' | 'register'>('login')
const account = ref({ username: '', password: '' })
const accountBusy = ref(false)
const accountError = ref('')
const links = ref<ResourceItem['links'] | null>(null)
const linkBusy = ref(false)

onMounted(async () => {
  try {
    const resources = await blogApi.listResources()
    item.value = resources.find(resource => resource.id === Number(route.params.id)) || null
    if (!item.value) error.value = '资源不存在或已下架'
  } catch { error.value = '资源加载失败，请稍后刷新' }
  finally { loading.value = false }
})

async function loadLinks(): Promise<ResourceItem['links'] | null> {
  if (!item.value) return null
  if (links.value) return links.value
  linkBusy.value = true
  try {
    const result = await blogApi.resourceAccess(item.value.id)
    links.value = result.links
    return result.links
  } catch {
    if (!getReaderToken()) { readerName.value = null; showAuth.value = true }
    notice.value = '获取链接失败，请重新登录后重试'
    return null
  } finally { linkBusy.value = false }
}

async function submitAccount() {
  accountError.value = ''
  accountBusy.value = true
  try {
    const result = accountMode.value === 'login'
      ? await blogApi.readerLogin(account.value.username, account.value.password)
      : await blogApi.readerRegister(account.value.username, account.value.password)
    setReaderSession(result.token, result.username)
    readerName.value = result.username
    account.value.password = ''
    showAuth.value = false
    notice.value = '登录成功，请点击网盘按钮继续'
  } catch { accountError.value = accountMode.value === 'login' ? '账号或密码错误' : '注册失败，请检查用户名和密码' }
  finally { accountBusy.value = false }
}
function logout() {
  clearReaderSession()
  readerName.value = null
  links.value = null
}
function fallbackCopy(value: string): boolean {
  const input = document.createElement('textarea')
  input.value = value
  input.style.position = 'fixed'
  input.style.opacity = '0'
  document.body.append(input)
  input.select()
  const copied = document.execCommand('copy')
  input.remove()
  return copied
}
async function copy(value: string): Promise<boolean> {
  try {
    if (navigator.clipboard?.writeText) { await navigator.clipboard.writeText(value); return true }
  } catch { /* 使用兼容方式继续尝试 */ }
  return fallbackCopy(value)
}
async function copyAndOpen(index: number) {
  if (!getReaderToken()) { readerName.value = null; showAuth.value = true; return }
  const tab = window.open('', '_blank')
  const available = await loadLinks()
  const link = available?.[index]
  if (!link) { tab?.close(); return }
  const copied = await copy(link.code || link.url)
  notice.value = copied ? (link.code ? '提取码已复制' : '链接已复制') : '浏览器未允许复制，请手动复制'
  if (tab) tab.location.href = link.url
  else window.location.href = link.url
}
async function copyAddress(index: number) {
  if (!getReaderToken()) { readerName.value = null; showAuth.value = true; return }
  const available = await loadLinks()
  const link = available?.[index]
  if (!link) return
  notice.value = await copy(link.url) ? '下载链接已复制' : '浏览器未允许复制，请手动复制'
}
</script>

<template>
  <main class="container resource-detail">
    <router-link to="/resources" class="back">← 返回资源列表</router-link>
    <p v-if="loading" class="state">加载中…</p>
    <p v-else-if="error" class="state" role="alert">{{ error }}</p>
    <template v-else-if="item">
      <div class="hero-image"><img v-if="item.image" :src="item.image" :alt="item.title" /><span v-else>~/resource</span></div>
      <div class="eyebrow">RESOURCE / {{ String(item.id).padStart(2, '0') }}</div>
      <h1>{{ item.title }}</h1>
      <p class="description">{{ item.description }}</p>
      <section class="download">
        <div class="download-head"><h2>获取资源</h2><span v-if="readerName">已登录：{{ readerName }} <button type="button" @click="logout">退出</button></span></div>
        <p>选择网盘后复制提取码并跳转；右键按钮可复制下载链接。</p>
        <div class="buttons">
          <button v-for="(label, index) in item.linkLabels || []" :key="index" type="button" :disabled="linkBusy" @click="copyAndOpen(index)" @contextmenu.prevent="copyAddress(index)">{{ label }} ↗</button>
          <span v-if="!item.linkLabels?.length">暂无下载链接</span>
        </div>
      </section>
      <section v-if="showAuth && !readerName" class="auth" aria-label="读者登录">
        <h2>{{ accountMode === 'login' ? '登录后获取链接' : '注册读者账号' }}</h2>
        <form @submit.prevent="submitAccount">
          <input v-model="account.username" aria-label="用户名" placeholder="用户名" autocomplete="username" required />
          <input v-model="account.password" type="password" aria-label="密码" placeholder="密码（至少 8 位）" :autocomplete="accountMode === 'login' ? 'current-password' : 'new-password'" required />
          <button type="submit" :disabled="accountBusy">{{ accountBusy ? '请稍候…' : accountMode === 'login' ? '登录' : '注册并登录' }}</button>
        </form>
        <button class="mode-btn" type="button" @click="accountMode = accountMode === 'login' ? 'register' : 'login'">{{ accountMode === 'login' ? '没有账号？注册' : '已有账号？登录' }}</button>
        <p v-if="accountError" role="alert">{{ accountError }}</p>
      </section>
    </template>
    <p v-if="notice" class="notice" role="status">{{ notice }}</p>
  </main>
</template>

<style scoped>
.resource-detail { max-width: 850px; padding-top: 46px; padding-bottom: 80px; }
.back { color: var(--accent); font-size: 13px; }
.state { margin-top: 30px; color: var(--ink-soft); }
.hero-image { display: flex; align-items: center; justify-content: center; height: 300px; margin: 26px 0; border-radius: var(--radius-lg); overflow: hidden; background: var(--bg-inset); color: var(--amber); font-family: var(--font-mono); }
.hero-image img { width: 100%; height: 100%; object-fit: cover; }
.eyebrow { color: var(--amber); }
h1 { margin: 10px 0 18px; font-size: 36px; }
.description { white-space: pre-wrap; color: var(--ink-soft); font-size: 16px; line-height: 1.8; }
.download, .auth { margin-top: 30px; padding: 22px; border: 1px solid var(--line); border-radius: var(--radius-lg); background: var(--bg-raised); }
.download-head { display: flex; justify-content: space-between; gap: 12px; align-items: baseline; }
.download-head h2, .auth h2 { font-size: 21px; }
.download-head span, .download > p { color: var(--ink-soft); font-size: 13px; }
.download-head button, .mode-btn { color: var(--accent); background: transparent; }
.buttons, form { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 18px; }
.buttons button, form button { padding: 10px 16px; color: white; background: var(--accent); border-radius: var(--radius); }
.buttons button:hover, form button:hover { background: var(--accent-hover); }
input { min-width: 170px; padding: 10px 12px; border: 1px solid var(--line); border-radius: var(--radius); background: var(--bg); color: var(--ink); }
.mode-btn { margin-top: 16px; padding: 0; }
.auth p { margin-top: 12px; color: #b53b39; }
.notice { position: fixed; right: 22px; bottom: 22px; z-index: 100; padding: 10px 16px; color: white; background: var(--hl-bg); border-radius: var(--radius); box-shadow: var(--shadow-card); }
@media(max-width:650px) { .hero-image { height: 210px; } h1 { font-size: 29px; } }
</style>
