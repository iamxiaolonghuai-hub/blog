<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue'
import { adminApi, type BackupInfo, type BackupSettings } from '@/api/admin'

const info = ref<BackupInfo | null>(null)
const settings = ref<BackupSettings>({ enabled: false, time: '03:00', retentionDays: 30, remoteEnabled: false, remoteHost: '', remoteUser: '', remotePath: '', remotePort: 22 })
const busy = ref(false)
const error = ref('')
const message = ref('')
const restoreFile = ref<File | null>(null)
let timer: ReturnType<typeof setInterval> | undefined
async function refresh() {
  try {
    const result = await adminApi.backupInfo()
    info.value = result
    settings.value = { ...settings.value, ...result.settings }
  } catch { error.value = '备份状态加载失败' }
}
onMounted(() => { refresh(); timer = setInterval(refresh, 15000) })
onUnmounted(() => { if (timer) clearInterval(timer) })
async function save() {
  busy.value = true; error.value = ''; message.value = ''
  try { await adminApi.saveBackupSettings(settings.value); message.value = '备份计划已保存'; await refresh() }
  catch { error.value = '保存失败，请检查时间及远程服务器参数' }
  finally { busy.value = false }
}
async function run() {
  busy.value = true; error.value = ''; message.value = ''
  try { await adminApi.runBackup(); message.value = '备份已启动，完成后会出现在下方列表'; setTimeout(refresh, 2000) }
  catch { error.value = '启动备份失败' }
  finally { busy.value = false }
}
async function download(name: string) {
  busy.value = true; error.value = ''
  try {
    const blob = await adminApi.downloadBackup(name)
    const url = URL.createObjectURL(blob)
    const anchor = document.createElement('a')
    anchor.href = url; anchor.download = name; anchor.click()
    setTimeout(() => URL.revokeObjectURL(url), 60000)
  } catch { error.value = '下载失败，请重试' }
  finally { busy.value = false }
}
function selected(event: Event) { restoreFile.value = (event.target as HTMLInputElement).files?.[0] ?? null }
async function restore() {
  if (!restoreFile.value) return
  if (!confirm('恢复会覆盖当前数据库和上传文件。系统会先生成一份恢复前备份，确定继续吗？')) return
  busy.value = true; error.value = ''; message.value = ''
  try { await adminApi.restoreBackup(restoreFile.value); message.value = '恢复已启动，请等待状态更新'; setTimeout(refresh, 2000) }
  catch { error.value = '上传或恢复启动失败' }
  finally { busy.value = false }
}
function size(bytes: number) { return bytes < 1024 * 1024 ? `${Math.round(bytes / 1024)} KB` : `${(bytes / 1024 / 1024).toFixed(1)} MB` }
</script>

<template>
  <div class="backups">
    <h1>完整备份与恢复</h1>
    <p class="intro">完整备份包含 MySQL 数据库、文章与评论、站点设置、资源信息，以及上传的图片和附件。下载 ZIP 可保存到你的电脑。</p>
    <p v-if="error" class="error" role="alert">{{ error }}</p>
    <p v-if="message" class="message" role="status">{{ message }}</p>
    <section class="panel">
      <h2>立即备份</h2>
      <p>在服务器生成 ZIP，然后从下方下载。</p>
      <button :disabled="busy || info?.status.state === 'running'" @click="run">{{ info?.status.state === 'running' ? '任务进行中…' : '生成完整备份' }}</button>
      <p v-if="info?.status.message" class="status">最近任务：{{ info.status.message }}</p>
    </section>
    <section class="panel">
      <h2>定时备份</h2>
      <p>每天按北京时间运行，备份保存在云服务器的独立持久化卷中。</p>
      <label class="check"><input v-model="settings.enabled" type="checkbox" /> 启用每日备份</label>
      <div class="fields"><label>运行时间 <input v-model="settings.time" type="time" /></label><label>本地保留天数 <input v-model.number="settings.retentionDays" type="number" min="1" max="365" /></label></div>
      <label class="check"><input v-model="settings.remoteEnabled" type="checkbox" /> 同步到另一台 SSH 服务器</label>
      <div v-if="settings.remoteEnabled" class="remote">
        <div class="fields"><label>主机名或 IP <input v-model="settings.remoteHost" placeholder="backup.example.com" /></label><label>SSH 用户 <input v-model="settings.remoteUser" placeholder="ubuntu" /></label></div>
        <div class="fields"><label>远程目录 <input v-model="settings.remotePath" placeholder="/home/ubuntu/blog-backups" /></label><label>SSH 端口 <input v-model.number="settings.remotePort" type="number" min="1" max="65535" /></label></div>
        <p>将下方公钥加入目标服务器该用户的 <code>~/.ssh/authorized_keys</code>。首次同步会记录目标主机密钥。</p>
        <code class="key">{{ info?.remotePublicKey || '正在生成公钥…' }}</code>
      </div>
      <button :disabled="busy" @click="save">保存计划</button>
    </section>
    <section class="panel">
      <h2>备份文件</h2>
      <p v-if="!info?.files.length">暂无完整备份。</p>
      <div v-for="file in [...(info?.files ?? [])].sort((a,b) => b.name.localeCompare(a.name))" :key="file.name" class="file-row"><span>{{ file.name }}</span><span>{{ size(file.bytes) }}</span><button :disabled="busy" @click="download(file.name)">下载到电脑</button></div>
    </section>
    <section class="panel restore">
      <h2>上传并恢复</h2>
      <p>上传本站生成的完整备份 ZIP（最大 200MB）。恢复会覆盖当前数据库和上传文件，并先生成一份恢复前快照。</p>
      <input type="file" accept=".zip,application/zip" @change="selected" />
      <button :disabled="busy || !restoreFile" @click="restore">上传并恢复</button>
    </section>
  </div>
</template>

<style scoped>
.backups { max-width: 900px; } h1 { font-size: 25px; margin-bottom: 12px; } h2 { font-size: 18px; margin-bottom: 8px; } .intro,.panel p { color:var(--ink-soft); font-size: 14px; margin-bottom: 15px; } .panel { background:var(--bg-raised); border:1px solid var(--line); border-radius:var(--radius-lg); padding:22px; margin:18px 0; } button { padding:9px 16px; border:0; border-radius:var(--radius); background:var(--accent); color:#fff; cursor:pointer; } button:disabled { opacity:.55; cursor:wait; } label { display:block; font-size:13px; color:var(--ink-soft); } .check { margin:12px 0; } .fields { display:grid; grid-template-columns:1fr 1fr; gap:14px; margin:12px 0; } .fields input { display:block; width:100%; padding:8px; border:1px solid var(--line-strong); border-radius:var(--radius); margin-top:5px; } .remote { background:var(--bg-inset); padding:15px; margin:12px 0; } .key { display:block; overflow-wrap:anywhere; padding:10px; background:#fff; font-size:11px; } .file-row { display:flex; align-items:center; gap:12px; border-top:1px solid var(--line); padding:10px 0; font-size:13px; } .file-row span:first-child { flex:1; overflow-wrap:anywhere; } .status { margin-top:10px; } .error { color:#b3351f; } .message { color:var(--accent); } .restore input { display:block; margin-bottom:14px; } @media(max-width:600px) { .fields { grid-template-columns:1fr; } .file-row { flex-wrap:wrap; } }
</style>
