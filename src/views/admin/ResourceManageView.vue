<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { adminApi } from '@/api/admin'
import type { ResourceItem } from '@/api/types'

type Input = Omit<ResourceItem, 'id'>
const items = ref<ResourceItem[]>([])
const form = ref<Input>({ title: '', description: '', image: '', visible: true, links: [] })
const editId = ref<number | null>(null)
const editing = ref(false)
const busy = ref(false)
const error = ref('')
async function load() { try { items.value = await adminApi.listResources(); error.value = '' } catch { error.value = '资源加载失败' } }
function start(item?: ResourceItem) {
  editId.value = item?.id ?? null
  form.value = item ? { title: item.title, description: item.description, image: item.image, visible: item.visible, links: item.links.map(x => ({ ...x })) } : { title: '', description: '', image: '', visible: true, links: [] }
  editing.value = true
}
async function save() {
  if (!form.value.title.trim()) { error.value = '请填写资源名称'; return }
  if (form.value.links.some(x => !x.label.trim() || !/^https:\/\//.test(x.url))) { error.value = '网盘名称不能为空，链接需以 https:// 开头'; return }
  if (form.value.links.some(x => /\s|提取码|%20/i.test(x.url))) { error.value = '链接里包含空格或提取码说明。请只填网址，把提取码填在右侧单独的输入框'; return }
  busy.value = true
  try {
    if (editId.value) await adminApi.updateResource(editId.value, form.value)
    else await adminApi.createResource(form.value)
    editing.value = false
    await load()
  } catch { error.value = '保存失败，请检查内容和后端服务' }
  finally { busy.value = false }
}
async function remove(id: number) { if (!confirm('确定删除这条资源吗？')) return; await adminApi.deleteResource(id); await load() }
async function imageChange(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  busy.value = true
  try { form.value.image = (await adminApi.uploadResourceImage(file)).url }
  catch { error.value = '图片上传失败（单次请求上限 256MB）' }
  finally { busy.value = false; input.value = '' }
}
onMounted(load)
</script>

<template>
  <div class="manage">
    <header><h1>资源管理</h1><button @click="start()">+ 新建资源</button></header>
    <p v-if="error" role="alert">{{ error }}</p>
    <div v-if="editing" class="editor">
      <h2>{{ editId ? '编辑资源' : '新建资源' }}</h2>
      <label>名称<input v-model="form.title" maxlength="200" /></label>
      <label>介绍文本<textarea v-model="form.description" rows="5" /></label>
      <label>图片<input type="file" accept="image/png,image/jpeg,image/webp,image/gif,image/avif" @change="imageChange" /></label>
      <img v-if="form.image" :src="form.image" alt="资源预览" class="preview" />
      <button v-if="form.image" @click="form.image = ''">移除图片</button>
      <h3>访问链接</h3>
      <div v-for="(link, index) in form.links" :key="index" class="link-edit">
        <input v-model="link.label" placeholder="蓝奏云 / PikPak / 百度网盘" />
        <input v-model="link.url" placeholder="只填网址，例如 https://pan.baidu.com/s/...?pwd=xxxx" />
        <input v-model="link.code" placeholder="提取码（可选）" />
        <button @click="form.links.splice(index, 1)">删除</button>
      </div>
      <button @click="form.links.push({ label: '', url: '', code: '' })">+ 添加网盘链接</button>
      <label class="check"><input v-model="form.visible" type="checkbox" />前台显示</label>
      <div class="actions"><button :disabled="busy" @click="save">{{ busy ? '保存中…' : '保存' }}</button><button @click="editing = false">取消</button></div>
    </div>
    <article v-for="item in items" :key="item.id" class="entry">
      <img v-if="item.image" :src="item.image" alt="" />
      <div><h2>{{ item.title }} <small v-if="!item.visible">（已隐藏）</small></h2><p>{{ item.description }}</p><small>{{ item.links.length }} 个访问链接</small></div>
      <button @click="start(item)">编辑</button><button @click="remove(item.id)">删除</button>
    </article>
  </div>
</template>

<style scoped>
.manage { max-width: 1000px; } header { display:flex; justify-content:space-between; align-items:center; margin-bottom:20px; } h1 { font-size:24px; } button { padding:8px 13px; border:1px solid var(--line-strong); border-radius:var(--radius); background:#fff; color:var(--accent); cursor:pointer; } .editor,.entry { background:var(--bg-raised); border:1px solid var(--line); border-radius:var(--radius-lg); padding:20px; margin-bottom:16px; } .editor label { display:block; margin:12px 0; } .editor input:not([type=file]):not([type=checkbox]), textarea { width:100%; padding:9px; border:1px solid var(--line-strong); border-radius:var(--radius); } .preview { display:block; max-width:220px; max-height:160px; margin:10px 0; } .link-edit { display:grid; grid-template-columns:160px 1fr 150px auto; gap:8px; margin:8px 0; } .check { display:flex!important; gap:8px; } .actions { display:flex; gap:8px; margin-top:18px; } .entry { display:flex; align-items:center; gap:16px; } .entry img { width:90px; height:70px; object-fit:cover; } .entry div { flex:1; } .entry p { color:var(--ink-soft); white-space:pre-wrap; } small { color:var(--ink-faint); } [role=alert] { color:#b3351f; } @media(max-width:700px) { .link-edit { grid-template-columns:1fr; } }
</style>
