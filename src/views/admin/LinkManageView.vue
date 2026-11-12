<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { adminApi } from '@/api/admin'
import type { FriendLink, FriendLinkInput } from '@/api/types'
import ConfirmDialog from '@/components/admin/ConfirmDialog.vue'

const links = ref<FriendLink[]>([])
const loading = ref(true)
const loadError = ref('')
const editing = ref(false)
const isNew = ref(false)
const editId = ref<number | null>(null)
const deleteId = ref<number | null>(null)
const formError = ref('')

const emptyForm: FriendLinkInput = {
  name: '',
  url: '',
  description: '',
  logo: '',
  sortOrder: 1,
  visible: true,
}
const form = ref<FriendLinkInput>({ ...emptyForm })
const logoUploading = ref(false)

async function onLogoChange(e: Event) {
  const input = e.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  logoUploading.value = true
  try {
    const res = await adminApi.uploadLogo(file)
    form.value.logo = res.url
  } catch {
    formError.value = 'Logo 上传失败，请重试'
  } finally {
    logoUploading.value = false
    input.value = ''
  }
}

async function load() {
  loading.value = true
  try { links.value = await adminApi.listLinks(); loadError.value = '' }
  catch { loadError.value = '友链加载失败，请检查后端服务后重试' }
  finally { loading.value = false }
}

function openNew() {
  isNew.value = true
  editId.value = null
  formError.value = ''
  form.value = { ...emptyForm, sortOrder: links.value.length + 1 }
  editing.value = true
}

function openEdit(l: FriendLink) {
  isNew.value = false
  editId.value = l.id
  formError.value = ''
  form.value = {
    name: l.name,
    url: l.url,
    description: l.description ?? '',
    logo: l.logo ?? '',
    sortOrder: l.sortOrder,
    visible: l.visible,
  }
  editing.value = true
}

function close() {
  editing.value = false
}

async function submit() {
  formError.value = ''
  if (!form.value.name.trim()) {
    formError.value = '请填写链接名称'
    return
  }
  if (!form.value.url.trim()) {
    formError.value = '请填写链接地址'
    return
  }
  if (isNew.value) {
    await adminApi.createLink(form.value)
  } else if (editId.value !== null) {
    await adminApi.updateLink(editId.value, form.value)
  }
  close()
  await load()
}

async function toggleVisible(l: FriendLink) {
  await adminApi.updateLink(l.id, {
    name: l.name,
    url: l.url,
    description: l.description ?? '',
    sortOrder: l.sortOrder,
    visible: !l.visible,
  })
  await load()
}

async function confirmDelete() {
  if (deleteId.value !== null) {
    await adminApi.deleteLink(deleteId.value)
    deleteId.value = null
    await load()
  }
}

onMounted(load)
</script>

<template>
  <div class="lm">
    <div class="lm__head">
      <h1>友情链接</h1>
      <button class="lm__new" @click="openNew">+ 添加链接</button>
    </div>

    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
    <p v-else-if="loadError" role="alert">{{ loadError }}</p>

    <div v-else-if="!links.length" class="empty">
      <span class="empty__code">// no links</span>
      还没有友情链接，点击右上角添加。
    </div>

    <div v-else class="link-grid">
      <div v-for="l in links" :key="l.id" class="link-card" :class="{ 'is-hidden': !l.visible }">
        <div class="link-card__main">
          <a :href="l.url" target="_blank" rel="noopener noreferrer" class="link-card__name">{{ l.name }}</a>
          <span class="link-card__url">{{ l.url }}</span>
          <p class="link-card__desc">{{ l.description }}</p>
        </div>
        <div class="link-card__side">
          <span class="link-card__order">排序 {{ l.sortOrder }}</span>
          <button
            class="link-card__visible"
            :class="l.visible ? 'is-on' : 'is-off'"
            @click="toggleVisible(l)"
          >
            {{ l.visible ? '显示中' : '已隐藏' }}
          </button>
          <div class="link-card__actions">
            <button class="lact" @click="openEdit(l)">编辑</button>
            <button class="lact lact--danger" @click="deleteId = l.id">删除</button>
          </div>
        </div>
      </div>
    </div>

    <!-- 编辑弹窗 -->
    <teleport to="body">
      <div v-if="editing" class="edit-modal">
        <div class="edit-modal__mask" @click="close"></div>
        <div class="edit-modal__box">
          <h3>{{ isNew ? '添加链接' : '编辑链接' }}</h3>

          <label class="efield">
            <span>名称</span>
            <input v-model="form.name" type="text" placeholder="站点名称" />
          </label>
          <label class="efield">
            <span>URL</span>
            <input v-model="form.url" type="text" placeholder="https://example.com" />
          </label>
          <label class="efield">
            <span>描述</span>
            <input v-model="form.description" type="text" placeholder="一句话介绍" />
          </label>
          <div class="efield">
            <span>Logo</span>
            <div class="logo-upload">
              <img v-if="form.logo" :src="form.logo" alt="logo" class="logo-upload__img" />
              <label class="logo-upload__btn">
                <input type="file" accept="image/*" hidden @change="onLogoChange" />
                <span>{{ logoUploading ? '上传中…' : form.logo ? '更换' : '选择 Logo（≤2MB）' }}</span>
              </label>
              <button v-if="form.logo" type="button" class="logo-upload__remove" @click="form.logo = ''">移除</button>
            </div>
          </div>
          <div class="efield-row">
            <label class="efield efield--small">
              <span>排序</span>
              <input v-model.number="form.sortOrder" type="number" min="0" />
            </label>
            <label class="efield efield--check">
              <input v-model="form.visible" type="checkbox" />
              <span>前台可见</span>
            </label>
          </div>

          <p v-if="formError" class="edit-modal__error">{{ formError }}</p>

          <div class="edit-modal__actions">
            <button class="edit-modal__ghost" @click="close">取消</button>
            <button class="edit-modal__primary" @click="submit">保存</button>
          </div>
        </div>
      </div>
    </teleport>

    <ConfirmDialog
      :open="deleteId !== null"
      title="删除链接"
      message="确定删除这条友情链接吗？"
      @confirm="confirmDelete"
      @cancel="deleteId = null"
    />
  </div>
</template>

<style scoped>
.lm__head {
  display: flex; align-items: center; justify-content: space-between;
  margin-bottom: 22px;
}
.lm__head h1 { font-size: 24px; }
.lm__new {
  padding: 9px 18px; background: var(--accent);
  border: none; border-radius: var(--radius);
  font-size: 14px; color: #fff;
}
.lm__new:hover { background: var(--accent-hover); }

.link-grid { display: flex; flex-direction: column; gap: 12px; }
.link-card {
  display: flex; justify-content: space-between;
  gap: 18px; background: var(--bg-raised);
  border: 1px solid var(--line); border-radius: var(--radius-lg);
  padding: 16px 18px;
}
.link-card.is-hidden { opacity: 0.6; }
.link-card__name {
  font-size: 15.5px; font-weight: 600; color: var(--accent);
}
.link-card__url {
  display: block; font-family: var(--font-mono);
  font-size: 12px; color: var(--ink-faint); margin: 3px 0;
}
.link-card__desc { font-size: 13.5px; color: var(--ink-soft); margin: 0; }
.link-card__side {
  display: flex; flex-direction: column;
  align-items: flex-end; gap: 7px; white-space: nowrap;
}
.link-card__order {
  font-family: var(--font-mono); font-size: 11.5px; color: var(--ink-faint);
}
.link-card__visible {
  font-size: 12px; padding: 3px 10px;
  border-radius: 3px; border: 1px solid;
}
.link-card__visible.is-on { background: var(--accent-soft); color: var(--accent); border-color: transparent; }
.link-card__visible.is-off { background: var(--bg-inset); color: var(--ink-faint); border-color: var(--line-strong); }
.link-card__actions { display: flex; gap: 2px; }
.lact { background: none; border: none; font-size: 13px; color: var(--accent); padding: 3px 7px; }
.lact:hover { text-decoration: underline; }
.lact--danger { color: #b3351f; }

.edit-modal {
  position: fixed; inset: 0; z-index: 100;
  display: flex; align-items: center; justify-content: center; padding: 20px;
}
.edit-modal__mask { position: absolute; inset: 0; background: rgba(20,23,27,0.5); }
.edit-modal__box {
  position: relative; width: 100%; max-width: 440px;
  background: var(--bg-raised); border: 1px solid var(--line);
  border-radius: var(--radius-lg); padding: 24px;
}
.edit-modal__box h3 { font-size: 17px; margin-bottom: 18px; }
.efield { display: flex; flex-direction: column; margin-bottom: 14px; }
.efield span { font-size: 12.5px; color: var(--ink-faint); margin-bottom: 5px; }
.efield input[type="text"] {
  padding: 9px 11px; border: 1px solid var(--line-strong);
  border-radius: var(--radius); font-size: 13.5px; font-family: var(--font-mono);
}
.efield input:focus { outline: none; border-color: var(--accent); }
.logo-upload { display: flex; align-items: center; gap: 10px; }
.logo-upload__img { width: 40px; height: 40px; border-radius: 6px; object-fit: cover; border: 1px solid var(--line-strong); }
.logo-upload__btn {
  display: inline-flex; align-items: center; justify-content: center;
  padding: 7px 14px; border: 1px solid var(--line-strong);
  border-radius: var(--radius); font-size: 12.5px; color: var(--ink-soft);
  cursor: pointer; background: #fff;
}
.logo-upload__btn:hover { border-color: var(--accent); color: var(--accent); }
.logo-upload__remove { background: none; border: none; font-size: 12px; color: #b3351f; cursor: pointer; }
.efield-row { display: flex; gap: 14px; }
.efield--small { width: 110px; }
.efield--check { flex: 1; flex-direction: row; align-items: center; gap: 8px; justify-content: flex-end; padding-top: 20px; }
.efield--check span { margin: 0; }
.edit-modal__error {
  font-size: 13px; color: #b3351f;
  background: #f7e3dd; padding: 8px 12px; border-radius: var(--radius);
}
.edit-modal__actions { display: flex; justify-content: flex-end; gap: 10px; margin-top: 16px; }
.edit-modal__ghost {
  padding: 8px 16px; background: none;
  border: 1px solid var(--line-strong); border-radius: var(--radius);
  font-size: 14px; color: var(--ink);
}
.edit-modal__primary {
  padding: 8px 20px; background: var(--accent);
  border: none; border-radius: var(--radius);
  font-size: 14px; color: #fff;
}
</style>
