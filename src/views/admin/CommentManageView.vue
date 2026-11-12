<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { adminApi } from '@/api/admin'
import type { Comment, CommentStatus } from '@/api/types'
import { formatDate } from '@/utils/date'
import ConfirmDialog from '@/components/admin/ConfirmDialog.vue'

const emit = defineEmits<{ 'data-changed': [] }>()

type Tab = 'all' | CommentStatus
const tabs: { key: Tab; label: string }[] = [
  { key: 'all', label: '全部' },
  { key: 'pending', label: '待审' },
  { key: 'approved', label: '已通过' },
  { key: 'rejected', label: '已拒绝' },
]

const activeTab = ref<Tab>('all')
const comments = ref<Comment[]>([])
const loading = ref(true)
const loadError = ref('')
const deleteId = ref<number | null>(null)
const replyId = ref<number | null>(null)
const replyText = ref('')

async function load() {
  loading.value = true
  try { comments.value = await adminApi.listComments(activeTab.value === 'all' ? undefined : activeTab.value); loadError.value = '' }
  catch { loadError.value = '评论加载失败，请检查后端服务后重试' }
  finally { loading.value = false }
}

async function setStatus(id: number, status: CommentStatus) {
  await adminApi.setCommentStatus(id, status)
  await load()
  emit('data-changed')
}

function openReply(id: number) {
  replyId.value = id
  replyText.value = ''
}

async function submitReply() {
  if (!replyText.value.trim() || replyId.value === null) {
    replyId.value = null
    return
  }
  await adminApi.replyComment(replyId.value, replyText.value.trim())
  replyId.value = null
  await load()
  emit('data-changed')
}

async function confirmDelete() {
  if (deleteId.value !== null) {
    await adminApi.deleteComment(deleteId.value)
    deleteId.value = null
    await load()
    emit('data-changed')
  }
}

function switchTab(tab: Tab) {
  activeTab.value = tab
  load()
}

onMounted(load)
</script>

<template>
  <div class="cm">
    <div class="cm__head">
      <h1>评论管理</h1>
    </div>

    <div class="cm__tabs">
      <button
        v-for="t in tabs"
        :key="t.key"
        class="cm-tab"
        :class="{ 'is-active': activeTab === t.key }"
        @click="switchTab(t.key)"
      >
        {{ t.label }}
      </button>
    </div>

    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
    <p v-else-if="loadError" role="alert">{{ loadError }}</p>

    <div v-else-if="!comments.length" class="empty">
      <span class="empty__code">// no comments</span>
      该分类下没有评论。
    </div>

    <div v-else class="comment-list">
      <div v-for="c in comments" :key="c.id" class="comment-card" :class="`is-${c.status}`">
        <div class="comment-card__head">
          <span class="comment-card__author">{{ c.author }}</span>
          <a v-if="c.website" :href="c.website" target="_blank" rel="noopener" class="comment-card__web">{{ c.website }}</a>
          <span class="comment-card__post">评论于《{{ c.postTitle }}》</span>
          <span class="comment-card__status">{{ c.status }}</span>
        </div>
        <p class="comment-card__content">{{ c.content }}</p>
        <div class="comment-card__foot">
          <span class="comment-card__date">{{ formatDate(c.createdAt) }}</span>
          <div class="comment-card__actions">
            <router-link class="cact" :to="`/admin/comments/${c.id}/context`">查看上下文</router-link>
            <button v-if="c.status !== 'approved'" class="cact cact--ok" @click="setStatus(c.id, 'approved')">通过</button>
            <button v-if="c.status !== 'rejected' && c.author !== '管理员'" class="cact" @click="setStatus(c.id, 'rejected')">拒绝</button>
            <button class="cact" @click="openReply(c.id)">回复</button>
            <button class="cact cact--danger" @click="deleteId = c.id">删除</button>
          </div>
        </div>
      </div>
    </div>

    <!-- 回复弹窗 -->
    <teleport to="body">
      <div v-if="replyId !== null" class="reply-modal">
        <div class="reply-modal__mask" @click="replyId = null"></div>
        <div class="reply-modal__box">
          <h3>回复评论</h3>
          <textarea v-model="replyText" rows="4" class="reply-modal__input" placeholder="输入回复内容…"></textarea>
          <div class="reply-modal__actions">
            <button class="reply-modal__ghost" @click="replyId = null">取消</button>
            <button class="reply-modal__primary" @click="submitReply">发送回复</button>
          </div>
        </div>
      </div>
    </teleport>

    <ConfirmDialog
      :open="deleteId !== null"
      title="删除评论"
      message="确定删除这条评论吗？此操作不可恢复。"
      @confirm="confirmDelete"
      @cancel="deleteId = null"
    />
  </div>
</template>

<style scoped>
.cm__head h1 { font-size: 24px; margin-bottom: 16px; }
.cm__tabs { display: flex; gap: 6px; margin-bottom: 18px; flex-wrap: wrap; }
.cm-tab {
  padding: 6px 14px;
  background: none;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  font-size: 13px;
  color: var(--ink-soft);
}
.cm-tab.is-active { background: var(--accent); border-color: var(--accent); color: #fff; }

.comment-list { display: flex; flex-direction: column; gap: 14px; }
.comment-card {
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-left: 3px solid var(--line-strong);
  border-radius: var(--radius-lg);
  padding: 16px 18px;
}
.comment-card.is-pending { border-left-color: var(--amber); }
.comment-card.is-approved { border-left-color: var(--accent); }
.comment-card.is-rejected { border-left-color: #b3351f; opacity: 0.85; }

.comment-card__head {
  display: flex;
  align-items: baseline;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 8px;
}
.comment-card__author { font-weight: 600; font-size: 14.5px; }
.comment-card__web { font-size: 12px; color: var(--ink-faint); }
.comment-card__post { font-size: 12.5px; color: var(--ink-faint); }
.comment-card__status {
  margin-left: auto;
  font-family: var(--font-mono);
  font-size: 11px;
  color: var(--ink-faint);
}
.comment-card__content {
  font-size: 14.5px;
  color: var(--ink-soft);
  line-height: 1.7;
  margin-bottom: 12px;
}
.comment-card__foot {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 10px;
}
.comment-card__date {
  font-family: var(--font-mono);
  font-size: 12px;
  color: var(--ink-faint);
}
.comment-card__actions { display: flex; gap: 4px; }
.cact {
  background: none;
  border: none;
  font-size: 13px;
  padding: 4px 9px;
  border-radius: var(--radius);
  color: var(--accent);
}
.cact:hover { background: var(--accent-soft); }
.cact--ok { color: #2f7d4f; }
.cact--danger { color: #b3351f; }
.cact--danger:hover { background: #f7e3dd; }

.reply-modal {
  position: fixed; inset: 0; z-index: 100;
  display: flex; align-items: center; justify-content: center; padding: 20px;
}
.reply-modal__mask { position: absolute; inset: 0; background: rgba(20,23,27,0.5); }
.reply-modal__box {
  position: relative; width: 100%; max-width: 440px;
  background: var(--bg-raised); border: 1px solid var(--line);
  border-radius: var(--radius-lg); padding: 24px;
}
.reply-modal__box h3 { font-size: 17px; margin-bottom: 14px; }
.reply-modal__input {
  width: 100%; box-sizing: border-box;
  padding: 10px 12px; border: 1px solid var(--line-strong);
  border-radius: var(--radius); font-size: 14px; resize: vertical;
  font-family: var(--font-body);
}
.reply-modal__input:focus { outline: none; border-color: var(--accent); }
.reply-modal__actions { display: flex; justify-content: flex-end; gap: 10px; margin-top: 16px; }
.reply-modal__ghost {
  padding: 8px 16px; background: none;
  border: 1px solid var(--line-strong); border-radius: var(--radius);
  font-size: 14px; color: var(--ink);
}
.reply-modal__primary {
  padding: 8px 18px; background: var(--accent);
  border: none; border-radius: var(--radius);
  font-size: 14px; color: #fff;
}
</style>
