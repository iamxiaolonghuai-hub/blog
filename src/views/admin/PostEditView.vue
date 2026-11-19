<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { adminApi } from '@/api/admin'
import type { MusicTrack, PostInput, PostStatus } from '@/api/types'
import { renderMarkdown } from '@/utils/markdown'
import { parsePostMarkdown } from '@/utils/postMarkdown'

const route = useRoute()
const router = useRouter()

const isEdit = computed(() => !!route.params.id)
const saving = ref(false)
const error = ref('')

const form = ref<PostInput>({
  slug: '',
  title: '',
  summary: '',
  content: '',
  cover: '',
  tags: [],
  category: '',
  status: 'draft',
  musicId: 0,
  isPinned: false,
  isHot: false,
  displayWeight: 0,
})
const tagText = ref('')
const coverUploading = ref(false)
const coverError = ref('')
const assetUploading = ref(false)
const musicTracks = ref<MusicTrack[]>([])
async function onAssetChange(e: Event, image: boolean) {
  const input = e.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  assetUploading.value = true
  error.value = ''
  try {
    const { url } = image ? await adminApi.uploadArticleImage(file) : await adminApi.uploadAttachment(file)
    const label = file.name.replace(/[\[\]()]/g, '')
    form.value.content += `\n\n${image ? '!' : ''}[${label}](${url})\n`
  } catch { error.value = image ? '正文图片上传失败' : '附件上传失败（仅支持 pdf/txt/md/csv/zip，最大 10MB）' }
  finally { assetUploading.value = false; input.value = '' }
}

async function onMarkdownChange(e: Event) {
  const input = e.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  try {
    if (!/\.(md|markdown|txt|json)$/i.test(file.name) || file.size > 2 * 1024 * 1024)
      throw new Error('请选择不超过 2MB 的 MD、Markdown、TXT 或 JSON 文件')
    const source = await file.text()
    let imported: Partial<PostInput>
    if (/\.txt$/i.test(file.name)) {
      const content = source.replace(/\r\n/g, '\n').replace(/[&<>]/g, (char) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' })[char]!)
      imported = { title: file.name.replace(/\.txt$/i, ''), content }
    } else if (/\.json$/i.test(file.name)) {
      const data: unknown = JSON.parse(source)
      if (!data || typeof data !== 'object' || !('title' in data) || !('content' in data)
          || typeof data.title !== 'string' || typeof data.content !== 'string')
        throw new Error('JSON 需包含 title 和 content 字段（单篇文章）')
      const post = data as Partial<PostInput>
      imported = { title: post.title, content: post.content,
        slug: typeof post.slug === 'string' ? post.slug : '',
        summary: typeof post.summary === 'string' ? post.summary : '',
        category: typeof post.category === 'string' ? post.category : '',
        cover: typeof post.cover === 'string' ? post.cover : '',
        isPinned: post.isPinned === true, isHot: post.isHot === true,
        displayWeight: Number.isInteger(post.displayWeight) ? post.displayWeight : 0,
        tags: Array.isArray(post.tags) ? post.tags.filter((tag): tag is string => typeof tag === 'string') : [] }
    } else imported = parsePostMarkdown(source, file.name)
    form.value = { ...form.value, ...imported }
    tagText.value = imported.tags?.join(', ') ?? ''
    error.value = ''
  } catch (err) {
    error.value = err instanceof Error ? err.message : '导入失败'
  } finally { input.value = '' }
}

async function onCoverChange(e: Event) {
  const input = e.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  coverError.value = ''
  coverUploading.value = true
  try {
    const res = await adminApi.uploadCover(file)
    form.value.cover = res.url
  } catch {
    coverError.value = '封面上传失败，请重试'
  } finally {
    coverUploading.value = false
    input.value = ''
  }
}

type EditorMode = 'split' | 'edit' | 'preview'
const editorMode = ref<EditorMode>('split')

const previewHtml = computed(() => renderMarkdown(form.value.content).html)

function ensureSlug() {
  if (!form.value.slug) {
    form.value.slug = 'post-' + Date.now().toString(36)
  }
}

function parseTags() {
  form.value.tags = tagText.value
    .split(/[,，]/)
    .map((t) => t.trim())
    .filter(Boolean)
}

async function save(status: PostStatus) {
  error.value = ''
  if (!form.value.title.trim()) {
    error.value = '请填写文章标题'
    return
  }
  if (!form.value.content.trim()) {
    error.value = '正文不能为空'
    return
  }
  parseTags()
  if (!Number.isInteger(form.value.displayWeight ?? 0) || (form.value.displayWeight ?? 0) < -1000 || (form.value.displayWeight ?? 0) > 1000) {
    error.value = '展示权重须为 -1000 到 1000 的整数'
    return
  }
  ensureSlug()
  form.value.status = status
  saving.value = true
  try {
    if (isEdit.value) {
      await adminApi.updatePost(Number(route.params.id), form.value)
    } else {
      await adminApi.createPost(form.value)
    }
    router.push('/admin/posts')
  } catch {
    error.value = '保存失败，请稍后重试'
    saving.value = false
  }
}

onMounted(async () => {
  try { musicTracks.value = await adminApi.listMusic() }
  catch { /* 文章仍可编辑，曲目列表稍后重试 */ }
  if (isEdit.value) {
    // 用单篇接口取正文，不再拉全量列表
    try {
      const p = await adminApi.getPost(Number(route.params.id))
      form.value = {
        slug: p.slug,
        title: p.title,
        summary: p.summary,
        content: p.content,
        cover: p.cover ?? '',
        tags: p.tags,
        category: p.category ?? '',
        status: p.status,
        musicId: p.musicId ?? 0,
        isPinned: p.isPinned ?? false,
        isHot: p.isHot ?? false,
        displayWeight: p.displayWeight ?? 0,
      }
      tagText.value = p.tags.join(', ')
    } catch {
      error.value = '文章不存在或加载失败'
    }
  }
})
</script>

<template>
  <div class="pe">
    <div class="pe__head">
      <h1>{{ isEdit ? '编辑文章' : '写文章' }}</h1>
      <label class="pe__back">导入文章（MD/TXT/JSON） <input type="file" accept=".md,.markdown,.txt,.json" hidden @change="onMarkdownChange" /></label>
      <router-link to="/admin/posts" class="pe__back">← 返回列表</router-link>
    </div>

    <!-- 元信息 -->
    <div class="meta-form">
      <label class="field field--grow">
        <span class="field__label">标题</span>
        <input v-model="form.title" class="field__input" type="text" placeholder="文章标题" />
      </label>
      <label class="field">
        <span class="field__label">Slug（留空自动生成）</span>
        <input v-model="form.slug" class="field__input" type="text" placeholder="url-name" />
      </label>
      <label class="field">
        <span class="field__label">分类</span>
        <input v-model="form.category" class="field__input" type="text" placeholder="后端" />
      </label>
      <label class="field field--grow">
        <span class="field__label">标签（逗号分隔）</span>
        <input v-model="tagText" class="field__input" type="text" placeholder="vue, c++, 全栈" />
      </label>
      <label class="field field--full">
        <span class="field__label">摘要</span>
        <textarea v-model="form.summary" class="field__input field__textarea" rows="2" placeholder="一两句话概括文章"></textarea>
      </label>
      <label class="field field--full">
        <span class="field__label">文章背景音乐</span>
        <select v-model.number="form.musicId" class="field__input">
          <option :value="0">不指定（保持读者当前播放的音乐）</option>
          <option v-for="track in musicTracks" :key="track.id" :value="track.id">{{ track.title }}{{ track.artist ? ` · ${track.artist}` : '' }}</option>
        </select>
        <router-link to="/admin/music" class="field__hint">管理音乐库 ↗</router-link>
      </label>
      <div class="field field--full">
        <span class="field__label">展示设置</span>
        <div class="display-options">
          <label><input v-model="form.isPinned" type="checkbox" /> 置顶</label>
          <label><input v-model="form.isHot" type="checkbox" /> 热门</label>
          <label>展示权重 <input v-model.number="form.displayWeight" type="number" min="-1000" max="1000" step="1" class="field__input" /></label>
        </div>
        <span class="field__hint">置顶优先；权重越大越靠前。热门文章会显示标记。</span>
      </div>
      <div class="field field--full">
        <span class="field__label">封面图</span>
        <div class="cover-upload">
          <div v-if="form.cover" class="cover-upload__preview">
            <img :src="form.cover" alt="封面预览" />
            <button type="button" class="cover-upload__remove" @click="form.cover = ''">移除</button>
          </div>
          <label class="cover-upload__drop">
            <input type="file" accept="image/*" hidden @change="onCoverChange" />
            <span v-if="coverUploading">上传中…</span>
            <span v-else>点击选择封面图片（建议 16:9，单次请求上限 256MB）</span>
          </label>
        </div>
        <span v-if="coverError" class="cover-upload__error">{{ coverError }}</span>
      </div>
    </div>

    <!-- 编辑器 -->
    <div class="editor">
      <div class="editor__bar">
        <div class="editor__tabs">
          <button
            v-for="m in (['split','edit','preview'] as EditorMode[])"
            :key="m"
            class="editor__tab"
            :class="{ 'is-active': editorMode === m }"
            @click="editorMode = m"
          >
            {{ m === 'split' ? '分屏' : m === 'edit' ? '编辑' : '预览' }}
          </button>
        </div>
        <span class="editor__hint">支持 Markdown 语法</span>
        <div class="editor__assets">
          <label>插入图片<input type="file" accept="image/png,image/jpeg,image/gif,image/webp,image/avif" hidden :disabled="assetUploading" @change="onAssetChange($event, true)" /></label>
          <label>上传本地文件<input type="file" accept=".pdf,.txt,.md,.csv,.zip" hidden :disabled="assetUploading" @change="onAssetChange($event, false)" /></label>
          <span v-if="assetUploading">上传中…</span>
        </div>
      </div>

      <div class="editor__body" :class="`mode-${editorMode}`">
        <textarea
          v-model="form.content"
          class="editor__textarea"
          spellcheck="false"
          placeholder="在这里使用 Markdown 开始写作…"
        ></textarea>
        <div class="editor__preview markdown-body" v-html="previewHtml"></div>
      </div>
    </div>

    <p v-if="error" class="pe__error">{{ error }}</p>

    <div class="pe__actions">
      <button class="pe__btn-ghost" :disabled="saving" @click="save('draft')">存为草稿</button>
      <button v-if="isEdit && form.status !== 'draft'" class="pe__btn-ghost" :disabled="saving" @click="save('hidden')">保存并隐藏</button>
      <button class="pe__btn-primary" :disabled="saving" @click="save('published')">
        {{ saving ? '保存中…' : form.status === 'hidden' ? '恢复公开' : '发布文章' }}
      </button>
    </div>
  </div>
</template>

<style scoped>
.pe__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 20px;
}
.pe__head h1 { font-size: 22px; }
.pe__back { font-size: 13px; }

.meta-form {
  display: flex;
  flex-wrap: wrap;
  gap: 14px;
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  padding: 18px;
  margin-bottom: 18px;
}
.field { display: flex; flex-direction: column; min-width: 150px; }
.field--grow { flex: 1; }
.field--full { flex-basis: 100%; }
.field__label {
  font-size: 12px;
  color: var(--ink-faint);
  margin-bottom: 5px;
}
.field__input {
  padding: 8px 11px;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  font-size: 13.5px;
  font-family: var(--font-mono);
  background: #fff;
}
.field__input:focus {
  outline: none;
  border-color: var(--accent);
  box-shadow: 0 0 0 3px var(--accent-soft);
}
.field__textarea {
  resize: vertical;
  font-family: var(--font-body);
  font-size: 14px;
}
.field__hint { margin-top: 5px; font-size: 12px; }

.cover-upload { display: flex; gap: 12px; align-items: stretch; }
.cover-upload__preview {
  position: relative;
  width: 200px;
  border-radius: var(--radius);
  overflow: hidden;
  border: 1px solid var(--line-strong);
}
.cover-upload__preview img { width: 100%; height: 110px; object-fit: cover; display: block; }
.cover-upload__remove {
  position: absolute;
  right: 6px;
  top: 6px;
  border: none;
  background: rgba(0,0,0,0.6);
  color: #fff;
  font-size: 11px;
  padding: 2px 8px;
  border-radius: 4px;
  cursor: pointer;
}
.cover-upload__drop {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 1px dashed var(--line-strong);
  border-radius: var(--radius);
  background: #fff;
  color: var(--ink-faint);
  font-size: 13px;
  cursor: pointer;
  min-height: 110px;
}
.cover-upload__drop:hover { border-color: var(--accent); color: var(--accent); }
.cover-upload__error { font-size: 12px; color: #b3351f; margin-top: 4px; }

.editor {
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  overflow: hidden;
  background: var(--bg-raised);
}
.editor__bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 12px;
  background: var(--bg-inset);
  border-bottom: 1px solid var(--line);
}
.editor__tabs { display: flex; gap: 4px; }
.editor__tab {
  padding: 5px 13px;
  background: none;
  border: 1px solid transparent;
  border-radius: var(--radius);
  font-size: 12.5px;
  color: var(--ink-soft);
}
.editor__tab.is-active {
  background: var(--bg-raised);
  border-color: var(--line-strong);
  color: var(--ink);
}
.editor__hint { font-family: var(--font-mono); font-size: 11.5px; color: var(--ink-faint); }
.editor__assets { display: flex; gap: 12px; font-size: 12px; color: var(--accent); }
.editor__assets label { cursor: pointer; }

.editor__body {
  display: grid;
  grid-template-columns: 1fr 1fr;
  min-height: 420px;
}
.editor__textarea {
  border: none;
  resize: none;
  padding: 18px;
  font-family: var(--font-mono);
  font-size: 13.5px;
  line-height: 1.7;
  background: var(--bg-raised);
  color: var(--ink);
  border-right: 1px solid var(--line);
}
.editor__textarea:focus { outline: none; }
.editor__preview {
  padding: 18px 20px;
  overflow-y: auto;
  font-size: 15px;
}

.editor__body.mode-edit { grid-template-columns: 1fr; }
.editor__body.mode-edit .editor__preview { display: none; }
.editor__body.mode-edit .editor__textarea { border-right: none; }
.editor__body.mode-preview { grid-template-columns: 1fr; }
.editor__body.mode-preview .editor__textarea { display: none; }
.editor__body.mode-preview .editor__preview { display: block; }

.pe__error {
  margin-top: 14px;
  font-size: 13px;
  color: #b3351f;
  background: #f7e3dd;
  padding: 8px 12px;
  border-radius: var(--radius);
}
.pe__actions {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  margin-top: 18px;
}
.pe__btn-ghost {
  padding: 10px 20px;
  background: none;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  font-size: 14px;
  color: var(--ink);
}
.pe__btn-ghost:hover { border-color: var(--accent); color: var(--accent); }
.pe__btn-primary {
  padding: 10px 24px;
  background: var(--accent);
  border: none;
  border-radius: var(--radius);
  font-size: 14px;
  color: #fff;
}
.pe__btn-primary:hover { background: var(--accent-hover); }
.pe__btn-primary:disabled, .pe__btn-ghost:disabled { opacity: 0.6; cursor: wait; }
.display-options { display: flex; flex-wrap: wrap; align-items: center; gap: 18px; }
.display-options label { display: inline-flex; align-items: center; gap: 6px; font-size: 13px; }
.display-options input[type=number] { width: 90px; padding: 6px 8px; }
</style>
