<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { adminApi } from '@/api/admin'
import type { Post } from '@/api/types'
import { formatDate } from '@/utils/date'
import ConfirmDialog from '@/components/admin/ConfirmDialog.vue'
import { downloadText, serializePostMarkdown } from '@/utils/postMarkdown'

type Filter = 'all' | 'published' | 'hidden' | 'draft'

const posts = ref<Post[]>([])
const loading = ref(true)
const loadError = ref('')
const filter = ref<Filter>('all')
const query = ref('')
const month = ref('')
const tag = ref('')
const sort = ref('display')
const page = ref(1)
const pageSize = 20
const savingId = ref<number | null>(null)
const displayError = ref('')
const deleteId = ref<number | null>(null)
const backupError = ref('')

async function exportPost(id: number) {
  try {
    const post = await adminApi.getPost(id)
    downloadText(`${post.slug || `post-${id}`}.md`, serializePostMarkdown(post))
    backupError.value = ''
  } catch { backupError.value = '导出失败，请重试' }
}


const filters: { key: Filter; label: string }[] = [
  { key: 'all', label: '全部' },
  { key: 'published', label: '已发布' },
  { key: 'hidden', label: '已隐藏' },
  { key: 'draft', label: '草稿' },
]

async function toggleHidden(p: Post) {
  savingId.value = p.id
  displayError.value = ''
  try {
    const updated = await adminApi.updatePostStatus(p.id, p.status === 'hidden' ? 'published' : 'hidden')
    posts.value = posts.value.map(item => item.id === p.id ? { ...item, ...updated } : item)
  } catch { displayError.value = `《${p.title}》的状态保存失败` }
  finally { savingId.value = null }
}

const tags = computed(() => [...new Set(posts.value.flatMap(p => p.tags))].sort((a, b) => a.localeCompare(b, 'zh-CN')))
const filtered = computed(() => {
  const words = query.value.trim().toLocaleLowerCase().split(/\s+/).filter(Boolean)
  return posts.value.filter(p =>
    (filter.value === 'all' || p.status === filter.value) &&
    (!tag.value || p.tags.includes(tag.value)) &&
    (!month.value || p.createdAt.startsWith(month.value)) &&
    words.every(word => [p.title, p.slug, p.summary, p.category ?? '', ...p.tags]
      .join(' ').toLocaleLowerCase().includes(word)),
  ).sort((a, b) => {
    if (sort.value === 'oldest') return a.createdAt.localeCompare(b.createdAt)
    if (sort.value === 'newest') return b.createdAt.localeCompare(a.createdAt)
    if (sort.value === 'views') return (b.views ?? 0) - (a.views ?? 0) || b.createdAt.localeCompare(a.createdAt)
    return Number(!!b.isPinned) - Number(!!a.isPinned) ||
      (b.displayWeight ?? 0) - (a.displayWeight ?? 0) ||
      Number(!!b.isHot) - Number(!!a.isHot) || b.createdAt.localeCompare(a.createdAt)
  })
})
const pageCount = computed(() => Math.max(1, Math.ceil(filtered.value.length / pageSize)))
const visiblePosts = computed(() => filtered.value.slice((page.value - 1) * pageSize, page.value * pageSize))

async function updateDisplay(p: Post, changes: Partial<Pick<Post, 'isPinned' | 'isHot' | 'displayWeight'>>) {
  const input = {
    isPinned: changes.isPinned ?? !!p.isPinned,
    isHot: changes.isHot ?? !!p.isHot,
    displayWeight: changes.displayWeight ?? p.displayWeight ?? 0,
  }
  if (!Number.isInteger(input.displayWeight) || input.displayWeight < -1000 || input.displayWeight > 1000) {
    displayError.value = '展示权重须为 -1000 到 1000 的整数'
    return
  }
  savingId.value = p.id
  displayError.value = ''
  try {
    const updated = await adminApi.updatePostDisplay(p.id, input)
    posts.value = posts.value.map(item => item.id === p.id ? { ...item, ...updated } : item)
  } catch { displayError.value = `《${p.title}》的展示设置保存失败` }
  finally { savingId.value = null }
}

async function load() {
  try { posts.value = await adminApi.listAllPosts(); loadError.value = '' }
  catch { loadError.value = '文章加载失败，请检查后端服务后刷新页面' }
  finally { loading.value = false }
}

function askDelete(id: number) {
  deleteId.value = id
}

async function confirmDelete() {
  if (deleteId.value !== null) {
    await adminApi.deletePost(deleteId.value)
    deleteId.value = null
    await load()
  }
}

onMounted(load)
</script>

<template>
  <div class="pm">
    <div class="pm__head">
      <h1>文章管理</h1>
      <router-link class="pm__new" to="/admin/backups">完整备份与恢复</router-link>
      <router-link to="/admin/posts/new" class="pm__new">+ 写文章</router-link>
    </div>
    <p v-if="backupError" role="alert">{{ backupError }}</p>
    <p v-if="displayError" role="alert">{{ displayError }}</p>

    <div class="pm__filters">
      <button
        v-for="f in filters"
        :key="f.key"
        class="filter-btn"
        :class="{ 'is-active': filter === f.key }"
        @click="filter = f.key; page = 1"
      >
        {{ f.label }}
      </button>
    </div>

    <div class="pm__search">
      <label>搜索文章 <input v-model="query" type="search" placeholder="标题、摘要、分类、标签或网址名" @input="page = 1" /></label>
      <label>标签 <select v-model="tag" @change="page = 1"><option value="">全部标签</option><option v-for="name in tags" :key="name" :value="name">{{ name }}</option></select></label>
      <label>创建月份 <input v-model="month" type="month" @change="page = 1" /></label>
      <label>排序 <select v-model="sort" @change="page = 1"><option value="display">前台展示顺序</option><option value="newest">最新创建</option><option value="oldest">最早创建</option><option value="views">浏览量最高</option></select></label>
      <button type="button" class="pm__reset" @click="query = ''; tag = ''; month = ''; sort = 'display'; filter = 'all'; page = 1">重置</button>
    </div>
    <p class="pm__count">找到 {{ filtered.length }} 篇文章，置顶优先；权重越大越靠前。</p>

    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
    <p v-else-if="loadError" role="alert">{{ loadError }}</p>

    <div v-else-if="!filtered.length" class="empty">
      <span class="empty__code">// no posts</span>
      {{ posts.length ? '没有符合条件的文章，请调整筛选条件。' : '这里还没有文章，点击右上角开始写作。' }}
    </div>

    <div v-else class="table-wrap">
      <table class="table">
        <thead>
          <tr>
            <th>标题</th>
            <th>状态</th>
            <th>分类</th>
            <th>展示设置</th>
            <th>更新时间</th>
            <th>浏览</th>
            <th class="col-actions">操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="p in visiblePosts" :key="p.id">
            <td class="cell-title">
              <router-link :to="`/admin/posts/${p.id}/edit`">{{ p.title }}</router-link>
              <span class="cell-slug">/{{ p.slug }}</span>
              <span v-for="name in p.tags" :key="name" class="cell-tag">#{{ name }}</span>
            </td>
            <td>
              <span class="status" :class="`is-${p.status}`">
                {{ p.status === 'published' ? '已发布' : p.status === 'hidden' ? '已隐藏' : '草稿' }}
              </span>
            </td>
            <td class="cell-muted">{{ p.category ?? '—' }}</td>
            <td class="cell-display">
              <label><input type="checkbox" :checked="!!p.isPinned" :disabled="savingId === p.id" @change="updateDisplay(p, { isPinned: !p.isPinned })" />置顶</label>
              <label><input type="checkbox" :checked="!!p.isHot" :disabled="savingId === p.id" @change="updateDisplay(p, { isHot: !p.isHot })" />热门</label>
              <label>权重 <input type="number" min="-1000" max="1000" step="1" :value="p.displayWeight ?? 0" :disabled="savingId === p.id" aria-label="展示权重" @change="updateDisplay(p, { displayWeight: Number(($event.target as HTMLInputElement).value) })" /></label>
            </td>
            <td class="cell-date">{{ formatDate(p.updatedAt) }}</td>
            <td class="cell-muted">{{ p.views ?? 0 }}</td>
            <td class="col-actions">
              <router-link :to="`/admin/posts/${p.id}/edit`" class="act">编辑</router-link>
              <button v-if="p.status !== 'draft'" class="act" :disabled="savingId === p.id" @click="toggleHidden(p)">{{ p.status === 'hidden' ? '恢复公开' : '隐藏' }}</button>
              <button class="act" @click="exportPost(p.id)">导出 MD</button>
              <button class="act act--danger" @click="askDelete(p.id)">删除</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div v-if="pageCount > 1" class="pm__pages"><button type="button" :disabled="page <= 1" @click="page--">上一页</button><span>{{ page }} / {{ pageCount }}</span><button type="button" :disabled="page >= pageCount" @click="page++">下一页</button></div>

    <ConfirmDialog
      :open="deleteId !== null"
      title="删除文章"
      message="确定删除这篇文章吗？此操作不可恢复，文章正文将一并删除。"
      @confirm="confirmDelete"
      @cancel="deleteId = null"
    />
  </div>
</template>

<style scoped>
.pm__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 20px;
}
.pm__head h1 { font-size: 24px; }
.pm__new {
  padding: 9px 18px;
  background: var(--accent);
  color: #fff;
  border-radius: var(--radius);
  font-size: 14px;
}
.pm__new:hover { background: var(--accent-hover); color: #fff; }

.pm__filters { display: flex; gap: 6px; margin-bottom: 12px; }
.pm__search { display: flex; flex-wrap: wrap; align-items: end; gap: 10px; margin-bottom: 10px; }
.pm__search label { display: grid; gap: 5px; color: var(--ink-soft); font-size: 12px; }
.pm__search input, .pm__search select { min-height: 36px; padding: 6px 9px; border: 1px solid var(--line-strong); border-radius: var(--radius); background: var(--bg-raised); color: var(--ink); }
.pm__search input[type=search] { min-width: 260px; }
.pm__reset, .pm__pages button { min-height: 36px; padding: 6px 12px; border: 1px solid var(--line-strong); border-radius: var(--radius); background: var(--bg-raised); color: var(--accent); }
.pm__count { margin: 0 0 14px; color: var(--ink-faint); font-size: 12px; }
.pm__pages { display: flex; justify-content: center; align-items: center; gap: 12px; margin-top: 16px; }
.pm__pages button:disabled { opacity: .45; }
.filter-btn {
  padding: 6px 14px;
  background: none;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  font-size: 13px;
  color: var(--ink-soft);
}
.filter-btn.is-active {
  background: var(--accent);
  border-color: var(--accent);
  color: #fff;
}

.table-wrap {
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  overflow-x: auto;
}
.table {
  width: 100%;
  border-collapse: collapse;
  font-size: 14px;
  min-width: 900px;
}
.table th {
  text-align: left;
  font-size: 12.5px;
  font-weight: 600;
  color: var(--ink-faint);
  padding: 12px 16px;
  background: var(--bg-inset);
  border-bottom: 1px solid var(--line);
}
.table td {
  padding: 12px 16px;
  border-bottom: 1px solid var(--line);
  vertical-align: middle;
}
.table tbody tr:last-child td { border-bottom: none; }
.cell-title a { color: var(--ink); font-weight: 500; }
.cell-title a:hover { color: var(--accent); }
.cell-slug {
  display: block;
  font-family: var(--font-mono);
  font-size: 11.5px;
  color: var(--ink-faint);
  margin-top: 2px;
}
.cell-muted { color: var(--ink-faint); }
.cell-tag { display: inline-block; margin: 5px 5px 0 0; color: var(--accent); font-size: 11px; }
.cell-display { min-width: 190px; }
.cell-display label { display: inline-flex; align-items: center; gap: 3px; margin: 0 7px 5px 0; font-size: 12px; white-space: nowrap; }
.cell-display input[type=number] { width: 55px; padding: 3px; border: 1px solid var(--line-strong); border-radius: 3px; }
.cell-date { font-family: var(--font-mono); font-size: 12.5px; color: var(--ink-soft); }

.status {
  font-size: 12px;
  padding: 2px 9px;
  border-radius: 3px;
  white-space: nowrap;
}
.status.is-published { background: var(--accent-soft); color: var(--accent); }
.status.is-draft { background: var(--bg-inset); color: var(--ink-faint); }
.status.is-hidden { background: var(--bg-inset); color: var(--ink-soft); }

.col-actions { white-space: nowrap; text-align: right; }
.act {
  background: none;
  border: none;
  font-size: 13px;
  color: var(--accent);
  padding: 2px 6px;
}
.act:hover { text-decoration: underline; }
.act--danger { color: #b3351f; }
</style>
