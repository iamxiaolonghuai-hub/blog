<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import type { Comment } from '@/api/types'
import CommentThread from './CommentThread.vue'
import { clearReaderSession, getReaderName, getReaderToken, setReaderSession } from '@/utils/auth'

const props = defineProps<{ slug: string }>()

const comments = ref<Comment[]>([])
const loading = ref(true)
const submitting = ref(false)
const submitted = ref(false)
const error = ref('')
const imageUploading = ref(false)
const readerName = ref(getReaderName())
const accountMode = ref<'login' | 'register'>('login')
const account = ref({ username: '', password: '' })
const accountBusy = ref(false)
const accountError = ref('')
const replyingTo = ref<Comment | null>(null)
const childrenByParent = computed(() => {
  const map = new Map<number, Comment[]>()
  for (const comment of comments.value) {
    if (comment.parentId != null) {
      const siblings = map.get(comment.parentId) ?? []
      siblings.push(comment)
      map.set(comment.parentId, siblings)
    }
  }
  return map
})
const rootComments = computed(() => comments.value.filter((c) => c.parentId == null || !comments.value.some((p) => p.id === c.parentId)))

const form = ref({
  email: '',
  website: '',
  content: '',
  imageUrl: '',
})
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
  } catch { accountError.value = accountMode.value === 'login' ? '账号或密码错误' : '注册失败，请检查用户名是否重复及密码长度' }
  finally { accountBusy.value = false }
}
function logout() { clearReaderSession(); readerName.value = null }

function replyTo(comment: Comment) {
  replyingTo.value = comment
  submitted.value = false
  document.getElementById('comment-form')?.scrollIntoView({ behavior: 'smooth', block: 'center' })
}

async function load() {
  loading.value = true
  try {
    comments.value = await blogApi.listComments(props.slug)
  } catch {
    comments.value = []
  }
  loading.value = false
}

async function onImageChange(e: Event) {
  const input = e.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  imageUploading.value = true
  error.value = ''
  try {
    const res = await blogApi.uploadCommentImage(file)
    form.value.imageUrl = res.url
  } catch {
    if (!getReaderToken()) readerName.value = null
    error.value = '图片上传失败，请重新登录后重试'
  } finally {
    imageUploading.value = false
    input.value = ''
  }
}

async function submit() {
  error.value = ''
  if (!getReaderToken()) {
    error.value = '请先登录评论账号'
    return
  }
  if (!form.value.content.trim()) {
    error.value = '请填写评论内容'
    return
  }
  // 若上传了图片，把图片以 Markdown 形式追加到正文
  let content = form.value.content
  if (form.value.imageUrl) {
    content += `\n\n![](${form.value.imageUrl})`
  }
  submitting.value = true
  try {
    await blogApi.submitComment(props.slug, {
      email: form.value.email,
      website: form.value.website,
      content,
      parentId: replyingTo.value?.id,
    })
    submitted.value = true
    replyingTo.value = null
    form.value = { email: '', website: '', content: '', imageUrl: '' }
  } catch {
    if (!getReaderToken()) readerName.value = null
    error.value = '提交失败，请检查登录状态后重试'
  } finally {
    submitting.value = false
  }
}

onMounted(load)
</script>

<template>
  <section class="cs">
    <h2 class="cs__title">评论 <span class="cs__count">{{ comments.length }}</span></h2>

    <div v-if="loading" class="cs__loading">加载中…</div>
    <div v-else class="cs__list">
      <div v-if="!comments.length" class="cs__empty">还没有评论，来写下第一条吧。</div>
      <CommentThread v-for="c in rootComments" :key="c.id" :comment="c" :children-by-parent="childrenByParent" @reply="replyTo" />
    </div>

    <div id="comment-form" class="cs__form">
      <h3 class="cs__form-title">{{ replyingTo ? `回复 ${replyingTo.author}` : '发表评论' }}</h3>
      <button v-if="replyingTo" class="cs__switch" @click="replyingTo = null">取消回复</button>
      <p v-if="submitted" class="cs__notice">评论已提交，待博主审核后显示。</p>
      <div v-if="!readerName || !getReaderToken()" class="cs__account">
        <p>登录账号后可以评论和上传图片。</p>
        <form @submit.prevent="submitAccount">
          <input v-model="account.username" class="cs__input" autocomplete="username" placeholder="用户名（3-30 位字母、数字或下划线）" required />
          <input v-model="account.password" class="cs__input" type="password" :autocomplete="accountMode === 'login' ? 'current-password' : 'new-password'" placeholder="密码（至少 8 位）" required />
          <button class="cs__submit" :disabled="accountBusy">{{ accountMode === 'login' ? '登录' : '注册并登录' }}</button>
        </form>
        <button class="cs__switch" @click="accountMode = accountMode === 'login' ? 'register' : 'login'">{{ accountMode === 'login' ? '没有账号？注册' : '已有账号？登录' }}</button>
        <p v-if="accountError" class="cs__error">{{ accountError }}</p>
      </div>
      <template v-else>
        <p class="cs__signed">以 {{ readerName }} 身份评论 <button @click="logout">退出</button></p>
        <div class="cs__row">
          <input v-model="form.email" class="cs__input" type="email" placeholder="邮箱（选填，不公开）" />
          <input v-model="form.website" class="cs__input" type="url" placeholder="网站（选填）" />
        </div>
        <textarea v-model="form.content" class="cs__textarea" rows="4" :placeholder="replyingTo ? `回复 ${replyingTo.author}…` : '写下你的看法…'"></textarea>
        <div class="cs__toolbar">
          <label class="cs__imgbtn">
            <input type="file" accept="image/*" hidden @change="onImageChange" />
            <span>{{ imageUploading ? '图片上传中…' : '插入图片' }}</span>
          </label>
          <span v-if="form.imageUrl" class="cs__imgok">已附加图片</span>
        </div>
        <p v-if="error" class="cs__error">{{ error }}</p>
        <button class="cs__submit" :disabled="submitting" @click="submit">
          {{ submitting ? '提交中…' : '提交评论' }}
        </button>
      </template>
    </div>
  </section>
</template>

<style scoped>
.cs { margin-top: 56px; border-top: 1px solid var(--line); padding-top: 28px; }
.cs__title { font-size: 20px; margin-bottom: 18px; }
.cs__count { color: var(--ink-faint); font-size: 15px; }
.cs__loading, .cs__empty { font-size: 14px; color: var(--ink-faint); padding: 12px 0; }
.cs__list { display: flex; flex-direction: column; gap: 18px; margin-bottom: 32px; }
.cs__item { padding-bottom: 16px; border-bottom: 1px solid var(--line); }
.cs__head { display: flex; align-items: baseline; gap: 10px; margin-bottom: 6px; }
.cs__author { font-weight: 600; font-size: 14.5px; }
.cs__date { font-family: var(--font-mono); font-size: 11.5px; color: var(--ink-faint); }
.cs__body { font-size: 14px; }
.cs__body :deep(img) { max-width: 100%; border-radius: 6px; }
.cs__form-title { font-size: 16px; margin-bottom: 12px; }
.cs__row { display: flex; gap: 12px; margin-bottom: 10px; }
.cs__input {
  flex: 1; padding: 9px 12px; border: 1px solid var(--line-strong);
  border-radius: var(--radius); font-size: 13.5px; background: #fff;
}
.cs__input:focus, .cs__textarea:focus { outline: none; border-color: var(--accent); }
.cs__textarea {
  width: 100%; padding: 11px 12px; border: 1px solid var(--line-strong);
  border-radius: var(--radius); font-size: 14px; resize: vertical; background: #fff;
}
.cs__toolbar { display: flex; align-items: center; gap: 10px; margin-top: 8px; }
.cs__imgbtn {
  display: inline-flex; padding: 6px 12px; border: 1px solid var(--line-strong);
  border-radius: var(--radius); font-size: 12.5px; color: var(--ink-soft); cursor: pointer; background: #fff;
}
.cs__imgbtn:hover { border-color: var(--accent); color: var(--accent); }
.cs__imgok { font-size: 12px; color: var(--accent); }
.cs__error { font-size: 13px; color: #b3351f; margin: 8px 0; }
.cs__submit {
  margin-top: 10px; padding: 9px 22px; background: var(--accent);
  border: none; border-radius: var(--radius); font-size: 14px; color: #fff; cursor: pointer;
}
.cs__submit:hover { background: var(--accent-hover); }
.cs__submit:disabled { opacity: 0.6; cursor: wait; }
.cs__notice {
  font-size: 14px; color: var(--accent); background: var(--accent-soft);
  padding: 12px 14px; border-radius: var(--radius);
}
.cs__account form { display: flex; gap: 8px; flex-wrap: wrap; align-items: start; margin-top: 10px; }
.cs__account .cs__input { min-width: 180px; }
.cs__account .cs__submit { margin-top: 0; }
.cs__switch, .cs__signed button { border: 0; background: none; color: var(--accent); cursor: pointer; padding: 8px 0; }
.cs__signed { font-size: 13px; margin-bottom: 10px; }
</style>
