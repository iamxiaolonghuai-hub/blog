<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { adminApi } from '@/api/admin'
import { clearToken } from '@/utils/auth'

const router = useRouter()
const currentPassword = ref('')
const newPassword = ref('')
const confirmation = ref('')
const busy = ref(false)
const error = ref('')

async function submit() {
  error.value = ''
  if (newPassword.value.length < 8 || newPassword.value.length > 128) {
    error.value = '新密码需为 8-128 位'
    return
  }
  if (newPassword.value !== confirmation.value) {
    error.value = '两次输入的新密码不一致'
    return
  }
  busy.value = true
  try {
    await adminApi.changePassword(currentPassword.value, newPassword.value)
    clearToken()
    window.alert('密码已更新，请使用新密码登录')
    await router.push('/admin/login')
  } catch {
    error.value = '修改失败，请检查旧密码后重试'
  } finally { busy.value = false }
}
</script>

<template>
  <section class="password-page">
    <h1>修改管理员密码</h1>
    <p>修改后请使用新密码重新登录。</p>
    <form @submit.prevent="submit">
      <label>当前密码<input v-model="currentPassword" type="password" autocomplete="current-password" required /></label>
      <label>新密码<input v-model="newPassword" type="password" autocomplete="new-password" minlength="8" maxlength="128" required /></label>
      <label>确认新密码<input v-model="confirmation" type="password" autocomplete="new-password" required /></label>
      <p v-if="error" role="alert" class="error">{{ error }}</p>
      <button type="submit" :disabled="busy">{{ busy ? '保存中…' : '保存新密码' }}</button>
    </form>
  </section>
</template>

<style scoped>
.password-page { max-width: 520px; }
.password-page h1 { margin-bottom: 8px; }
.password-page > p { color: var(--ink-soft); margin-bottom: 24px; }
form { display: grid; gap: 17px; padding: 24px; border: 1px solid var(--line); border-radius: var(--radius-lg); background: var(--bg-raised); }
label { display: grid; gap: 8px; font-size: 14px; }
input { width: 100%; padding: 10px 12px; border: 1px solid var(--line); border-radius: var(--radius); background: var(--bg); color: var(--ink); }
button { width: fit-content; padding: 10px 18px; color: white; background: var(--accent); border-radius: var(--radius); }
.error { color: #b53b39; }
</style>
