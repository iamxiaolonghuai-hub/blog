<script setup lang="ts">
import { nextTick, onMounted, ref } from 'vue'
import { adminApi } from '@/api/admin'
import { blogApi } from '@/api'
import type { MusicTrack } from '@/api/types'

const tracks = ref<MusicTrack[]>([])
const editingId = ref<number | null>(null)
const form = ref({ title: '', artist: '', url: '' })
const busy = ref(false)
const loading = ref(true)
const error = ref('')
const message = ref('')
const editor = ref<HTMLElement | null>(null)
const titleInput = ref<HTMLInputElement | null>(null)
const uploadProgress = ref<number | null>(null)
const uploading = ref(false)
const homeEnabled = ref(true)
const homeMusicId = ref(0)
const homeSaving = ref(false)
const homeMessage = ref('')

async function load() {
  try { tracks.value = await adminApi.listMusic(); error.value = '' }
  catch { error.value = '音乐列表加载失败' }
  finally { loading.value = false }
}
async function loadHomeMusic() {
  try {
    const site = await blogApi.getSite()
    homeEnabled.value = site.homeMusicEnabled !== false
    homeMusicId.value = site.homeMusicId ?? 0
  } catch { homeMessage.value = '首页音乐设置加载失败' }
}
async function saveHomeMusic() {
  homeSaving.value = true
  homeMessage.value = ''
  try {
    await adminApi.updateHomeMusic(homeEnabled.value, homeMusicId.value)
    homeMessage.value = '首页音乐设置已保存'
  } catch (e: any) { homeMessage.value = e?.response?.data?.message || '首页音乐设置保存失败' }
  finally { homeSaving.value = false }
}
async function edit(track: MusicTrack) {
  editingId.value = track.id
  form.value = { title: track.title, artist: track.artist, url: track.url }
  error.value = ''
  message.value = `正在编辑《${track.title}》`
  await nextTick()
  editor.value?.scrollIntoView({ behavior: 'smooth', block: 'start' })
  titleInput.value?.focus({ preventScroll: true })
}
function reset() { editingId.value = null; form.value = { title: '', artist: '', url: '' } }
async function uploadFile(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  if (!/\.(mp3|ogg|wav|m4a)$/i.test(file.name) || file.size > 50 * 1024 * 1024) {
    error.value = '请选择不超过 50MB 的 MP3、OGG、WAV 或 M4A 文件'
    input.value = ''
    return
  }
  busy.value = true
  uploading.value = true
  uploadProgress.value = 0
  error.value = ''
  try {
    form.value.url = (await adminApi.uploadMusic(file, (progress) => {
      if (progress.total) uploadProgress.value = Math.min(99, Math.round(progress.loaded / progress.total * 100))
    })).url
    uploadProgress.value = 100
    if (!form.value.title) form.value.title = file.name.replace(/\.[^.]+$/, '')
    message.value = '音频已上传，请填写曲名并保存'
  } catch (e: any) { error.value = e?.response?.data?.error || e?.response?.data?.message || e?.message || '音频上传失败，请检查文件后重试' }
  finally { busy.value = false; uploading.value = false; input.value = '' }
}
async function save() {
  if (!form.value.title.trim() || !form.value.url) { error.value = '请先上传音频并填写曲名'; return }
  busy.value = true
  error.value = ''
  try {
    const input = { title: form.value.title.trim(), artist: form.value.artist.trim(), url: form.value.url }
    if (editingId.value) await adminApi.updateMusic(editingId.value, input)
    else await adminApi.createMusic(input)
    reset()
    await load()
    message.value = '曲目已保存'
  } catch { error.value = '保存失败，请检查音频和曲目信息' }
  finally { busy.value = false }
}
async function remove(track: MusicTrack) {
  if (!window.confirm(`删除《${track.title}》吗？使用这首音乐的文章将不再关联它。`)) return
  try { await adminApi.deleteMusic(track.id); await load(); message.value = '曲目已删除' }
  catch { error.value = '删除失败，请稍后重试' }
}
onMounted(() => { void load(); void loadHomeMusic() })
</script>

<template>
  <div class="music-manage">
    <h1>音乐管理</h1>
    <p class="intro">上传音乐后，可在文章编辑页为每篇文章选择一首背景音乐。前台播放器支持顺序播放和单曲循环。</p>
    <section class="editor home-music">
      <h2>首页背景音乐</h2>
      <label class="home-switch"><input v-model="homeEnabled" type="checkbox" /> 在首页显示音乐播放器</label>
      <label>首页默认曲目
        <select v-model.number="homeMusicId" :disabled="!homeEnabled">
          <option :value="0">音乐库第一首</option>
          <option v-for="track in tracks" :key="track.id" :value="track.id">{{ track.title }}{{ track.artist ? ` · ${track.artist}` : '' }}</option>
        </select>
      </label>
      <div class="actions"><button type="button" :disabled="homeSaving" @click="saveHomeMusic">{{ homeSaving ? '保存中…' : '保存首页音乐' }}</button></div>
      <p v-if="homeMessage" role="status">{{ homeMessage }}</p>
    </section>
    <section ref="editor" class="editor">
      <h2>{{ editingId ? `编辑曲目：${tracks.find(track => track.id === editingId)?.title || form.title}` : '添加曲目' }}</h2>
      <label>音频文件 <input type="file" accept=".mp3,.ogg,.wav,.m4a,audio/*" :disabled="busy" @change="uploadFile" /></label>
      <div v-if="uploading" class="upload-progress" role="status" aria-live="polite">
        <div class="progress-label">正在上传音频… {{ uploadProgress === null ? '请稍候' : `${uploadProgress}%` }}</div>
        <progress :value="uploadProgress ?? undefined" max="100"></progress>
        <small v-if="uploadProgress === 99">文件已传送，等待服务器保存…</small>
      </div>
      <p v-if="form.url" class="uploaded">已上传：{{ form.url }}</p>
      <label>曲名 <input ref="titleInput" v-model="form.title" maxlength="200" placeholder="曲目名称" /></label>
      <label>艺术家（选填） <input v-model="form.artist" maxlength="120" placeholder="艺术家或来源" /></label>
      <div class="actions"><button type="button" :disabled="busy" @click="save">{{ busy ? '处理中…' : '保存曲目' }}</button><button v-if="editingId" type="button" class="ghost" @click="reset">取消编辑</button></div>
    </section>
    <p v-if="error" role="alert">{{ error }}</p>
    <p v-if="message" role="status">{{ message }}</p>
    <p v-if="loading">加载中…</p>
    <p v-else-if="!tracks.length">音乐库暂无曲目。</p>
    <div v-else class="track-list">
      <article v-for="track in tracks" :key="track.id" class="track">
        <span class="track-icon">♫</span>
        <div><strong>{{ track.title }}</strong><small>{{ track.artist || '未填写艺术家' }}</small></div>
        <audio :src="track.url" controls preload="none" :aria-label="`试听 ${track.title}`"></audio>
        <button type="button" class="ghost" @click="edit(track)">编辑</button>
        <button type="button" class="ghost danger" @click="remove(track)">删除</button>
      </article>
    </div>
  </div>
</template>

<style scoped>
.music-manage { max-width: 1000px; }
h1 { font-size: 24px; }
.intro { margin: 8px 0 20px; color: var(--ink-soft); font-size: 13px; }
.editor, .track { border: 1px solid var(--line); border-radius: var(--radius-lg); background: var(--bg-raised); }
.editor { display: grid; gap: 14px; padding: 20px; margin-bottom: 22px; }
.editor h2 { font-size: 17px; }
label { display: grid; gap: 6px; font-size: 13px; }
input:not([type=file]):not([type=checkbox]) { padding: 9px 11px; border: 1px solid var(--line-strong); border-radius: var(--radius); background: white; color: var(--ink); }
.home-music select { padding: 9px 11px; border: 1px solid var(--line-strong); border-radius: var(--radius); background: white; color: var(--ink); }
.home-switch { display: flex; align-items: center; gap: 8px; }
.uploaded { color: var(--ink-faint); font: 12px var(--font-mono); overflow-wrap: anywhere; }
.upload-progress { display: grid; gap: 6px; color: var(--ink-soft); font-size: 13px; }
.upload-progress progress { width: 100%; height: 10px; accent-color: var(--accent); }
.upload-progress small { color: var(--ink-faint); }
.actions { display: flex; gap: 10px; }
button { padding: 8px 14px; border-radius: var(--radius); color: white; background: var(--accent); }
button.ghost { color: var(--accent); border: 1px solid var(--line-strong); background: transparent; }
button.danger { color: #b3351f; }
button:disabled { opacity: .6; }
[role=alert] { color: #b3351f; margin-bottom: 14px; }
[role=status] { color: var(--accent); margin-bottom: 14px; }
.track-list { display: grid; gap: 10px; }
.track { display: flex; align-items: center; gap: 14px; padding: 12px 15px; }
.track-icon { font-size: 24px; color: var(--amber); }
.track > div { display: grid; gap: 4px; min-width: 150px; flex: 1; }
.track small { color: var(--ink-faint); }
.track audio { width: 250px; height: 34px; }
@media(max-width:760px) { .track { flex-wrap: wrap; } .track audio { order: 2; width: 100%; } }
</style>
