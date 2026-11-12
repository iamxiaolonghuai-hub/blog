<script setup lang="ts">
import { ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { adminApi } from '@/api/admin'
import { setToken } from '@/utils/auth'

const route = useRoute()
const router = useRouter()

const username = ref('')
const password = ref('')
const error = ref('')
const submitting = ref(false)

async function onSubmit() {
  if (!username.value || !password.value) {
    error.value = '请输入用户名和密码'
    return
  }
  submitting.value = true
  error.value = ''
  try {
    const res = await adminApi.login({
      username: username.value,
      password: password.value,
    })
    setToken(res.token)
    const redirect = (route.query.redirect as string) || '/admin'
    router.push(redirect)
  } catch {
    error.value = '用户名或密码错误'
  } finally {
    submitting.value = false
  }
}
</script>

<template>
  <div class="login">
    <form class="login__card" @submit.prevent="onSubmit">
      <p class="login__brand"><span class="login__prompt">~$</span> login</p>
      <h1 class="login__title">管理后台登录</h1>

      <label class="field">
        <span class="field__label">用户名</span>
        <input v-model="username" type="text" class="field__input" autocomplete="username" placeholder="admin" />
      </label>

      <label class="field">
        <span class="field__label">密码</span>
        <input v-model="password" type="password" class="field__input" autocomplete="current-password" placeholder="••••••" />
      </label>

      <p v-if="error" class="login__error">{{ error }}</p>

      <button type="submit" class="login__btn" :disabled="submitting">
        {{ submitting ? '登录中…' : '登 录' }}
      </button>
    </form>
  </div>
</template>

<style scoped>
.login {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
  background: var(--hl-bg);
}
.login__card {
  width: 100%;
  max-width: 360px;
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  padding: 34px 32px;
  box-shadow: 0 20px 60px rgba(0,0,0,0.3);
}
.login__brand {
  font-family: var(--font-mono);
  font-size: 14px;
  color: var(--ink);
  font-weight: 600;
  margin-bottom: 8px;
}
.login__prompt { color: var(--amber); }
.login__title {
  font-size: 22px;
  margin-bottom: 26px;
}
.field { display: block; margin-bottom: 18px; }
.field__label {
  display: block;
  font-size: 13px;
  color: var(--ink-soft);
  margin-bottom: 6px;
}
.field__input {
  width: 100%;
  padding: 10px 12px;
  font-family: var(--font-mono);
  font-size: 14px;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  background: #fff;
  color: var(--ink);
}
.field__input:focus {
  outline: none;
  border-color: var(--accent);
  box-shadow: 0 0 0 3px var(--accent-soft);
}
.login__error {
  font-size: 13px;
  color: #b3351f;
  background: #f7e3dd;
  padding: 8px 12px;
  border-radius: var(--radius);
  margin-bottom: 16px;
}
.login__btn {
  width: 100%;
  padding: 11px;
  background: var(--accent);
  color: #fff;
  border: none;
  border-radius: var(--radius);
  font-size: 15px;
  font-weight: 500;
  transition: background 0.15s ease;
}
.login__btn:hover { background: var(--accent-hover); }
.login__btn:disabled { opacity: 0.6; cursor: wait; }
.login__hint {
  margin-top: 18px;
  text-align: center;
  font-family: var(--font-mono);
  font-size: 12px;
  color: var(--ink-faint);
}
</style>
