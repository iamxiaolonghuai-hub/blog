<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { blogApi } from '@/api'
import { adminApi } from '@/api/admin'
import type { SiteContentSection, SiteInfo } from '@/api/types'
import { renderSafeHtml } from '@/utils/safeHtml'

const form = ref<SiteInfo>({ name: '', owner: '', title: '', bio: '', homeKicker: '', homeIntro: '', homeHeroTitle: '', homeTerminalTitle: '', homeMotto: '', profileSectionTitle: '', location: '', email: '', github: '' })
const loading = ref(true)
const saving = ref(false)
const avatarUploading = ref(false)
const avatarError = ref('')
const message = ref('')
const skills = ref<{ group: string; items: string }[]>([])
const sections = ref<SiteContentSection[]>([])
const pages = [{ key: 'about', label: '关于页' }] as const

async function uploadAvatar(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  avatarUploading.value = true
  avatarError.value = ''
  try {
    form.value.avatar = (await adminApi.uploadAvatar(file)).url
  } catch (error: any) {
    avatarError.value = error?.response?.data?.message || '头像上传失败，请重试'
  } finally {
    avatarUploading.value = false
    input.value = ''
  }
}

function addSection(page: 'home' | 'about') {
  sections.value.push({ id: `section-${Date.now()}-${Math.random().toString(36).slice(2)}`, page, slot: 'middle', title: '', html: '<p>在这里填写内容</p>', visible: true })
}
function moveSection(id: string, direction: -1 | 1) {
  const index = sections.value.findIndex(section => section.id === id)
  if (index < 0) return
  const page = sections.value[index].page
  let other = index + direction
  while (other >= 0 && other < sections.value.length && sections.value[other].page !== page) other += direction
  if (other >= 0 && other < sections.value.length) [sections.value[index], sections.value[other]] = [sections.value[other], sections.value[index]]
}

onMounted(async () => {
  try {
    form.value = await blogApi.getSite()
    skills.value = (form.value.skills ?? []).map((s) => ({ group: s.group, items: s.items.join('、') }))
    sections.value = (form.value.contentSections ?? []).filter(section => section.page === 'about').map(section => ({ ...section, slot: section.slot || 'middle' }))
  }
  catch { message.value = '站点资料加载失败' }
  finally { loading.value = false }
})

async function save() {
  saving.value = true
  message.value = ''
  try {
    form.value.skills = skills.value.filter((s) => s.group.trim()).map((s) => ({
      group: s.group.trim(),
      items: s.items.split(/[,，、\n]/).map((item) => item.trim()).filter(Boolean),
    }))
    form.value.contentSections = sections.value.map(section => ({ ...section }))
    await adminApi.updateSite(form.value)
    message.value = '已保存，刷新前台即可看到更新'
  } catch { message.value = '保存失败，请确认后端已更新并重试' }
  finally { saving.value = false }
}
</script>

<template>
  <div class="site-manage">
    <h1>站点资料</h1>
    <p v-if="loading">加载中…</p>
    <form v-else class="site-form" @submit.prevent="save">
      <label v-for="field in ([['name','博客名称'], ['owner','你的名字'], ['title','个人标题（关于页）'], ['location','所在地'], ['email','联系邮箱'], ['github','GitHub 地址']] as const)" :key="field[0]">
        <span>{{ field[1] }}</span>
        <input v-model="form[field[0]]" :type="field[0] === 'email' ? 'email' : 'text'" />
      </label>
      <label class="wide"><span>个人简介</span><textarea v-model="form.bio" rows="6" placeholder="介绍你自己…" /></label>
      <div class="wide avatar-editor">
        <span>个人头像（关于页）</span>
        <div class="avatar-editor__row">
          <img v-if="form.avatar" :src="form.avatar" alt="头像预览" />
          <span v-else class="avatar-editor__fallback">{{ form.owner?.charAt(0) || '肖' }}</span>
          <label class="avatar-editor__upload">{{ avatarUploading ? '上传中…' : '上传头像' }}<input type="file" accept=".jpg,.jpeg,.png,.gif,.webp,.avif,image/*" :disabled="avatarUploading" hidden @change="uploadAvatar" /></label>
          <button v-if="form.avatar" type="button" class="avatar-editor__remove" @click="form.avatar = ''">移除头像</button>
        </div>
        <small>支持 JPG、PNG、GIF、WebP、AVIF。上传后点击页面底部“保存资料”生效。</small>
        <p v-if="avatarError" role="alert">{{ avatarError }}</p>
      </div>
      <div class="wide section-head"><strong>首页内容</strong><p>下面的文字直接对应首页。主标题可换行；终端里的文章文件名会自动读取已发布文章。</p></div>
      <label class="wide"><span>首页顶部短句</span><input v-model="form.homeKicker" placeholder="个人博客 · personal blog" /><small>显示在首页大标题上方的橙色文字。</small></label>
      <label class="wide"><span>首页主标题</span><textarea v-model="form.homeHeroTitle" rows="3" placeholder="在终端与代码之间，&#10;记录构建系统的过程。" /><small>按回车换行，首页会保留换行。</small></label>
      <label class="wide"><span>首页终端个人介绍</span><input v-model="form.homeIntro" placeholder="肖龙淮 · C++ backend / Vue frontend" /><small>显示在首页终端 whoami 命令下面。</small></label>
      <label class="wide"><span>终端窗口标题</span><input v-model="form.homeTerminalTitle" placeholder="zsh — xiaolonghuai" /></label>
      <label class="wide"><span>终端座右铭</span><input v-model="form.homeMotto" placeholder="把想清楚的问题，写下来。" /></label>
      <div class="wide section-head"><strong>关于页展示分组</strong><p>可以写技术、阅读、旅行、作品或任何想分享的方向；删除全部分组后，这一块会隐藏。</p></div>
      <label class="wide"><span>分组区标题</span><input v-model="form.profileSectionTitle" placeholder="个人方向与兴趣" /></label>
      <div v-for="(skill, index) in skills" :key="index" class="wide skill-row">
        <input v-model="skill.group" :aria-label="`第 ${index + 1} 组名称`" placeholder="分组，如技术 / 阅读 / 旅行" />
        <textarea v-model="skill.items" :aria-label="`第 ${index + 1} 组内容`" rows="2" placeholder="每项用逗号、顿号或换行分隔" />
        <button type="button" @click="skills.splice(index, 1)">删除</button>
      </div>
      <button type="button" @click="skills.push({ group: '', items: '' })">添加分组</button>
      <div class="wide section-head"><strong>关于页内容模块</strong><p>可以添加多个模块，调整顺序或隐藏。内容支持常用 HTML 标签，预览会过滤脚本、内联样式和危险链接。</p></div>
      <div v-for="page in pages" :key="page.key" class="wide page-sections">
        <div class="page-sections__head"><h2>{{ page.label }}模块</h2><button type="button" @click="addSection(page.key)">+ 添加模块</button></div>
        <template v-for="section in sections" :key="section.id">
          <div v-if="section.page === page.key" class="module-editor">
            <div class="module-editor__head">
              <label class="check"><input v-model="section.visible" type="checkbox" />显示</label>
              <button type="button" @click="moveSection(section.id, -1)">上移</button>
              <button type="button" @click="moveSection(section.id, 1)">下移</button>
              <button type="button" @click="sections.splice(sections.findIndex(item => item.id === section.id), 1)">删除</button>
            </div>
            <label><span>模块标题</span><input v-model="section.title" maxlength="120" placeholder="可留空" /></label>
            <label><span>显示位置</span><select v-model="section.slot"><option value="top">页面顶部</option><option value="middle">主要内容之后</option><option value="bottom">页面底部</option></select></label>
            <label><span>HTML 内容</span><textarea v-model="section.html" rows="7" maxlength="20000" spellcheck="false" placeholder="<p>写一段介绍</p>" /></label>
            <div class="module-preview"><strong>预览</strong><div v-html="renderSafeHtml(section.html)"></div></div>
          </div>
        </template>
      </div>
      <p v-if="message" role="status">{{ message }}</p>
      <button type="submit" :disabled="saving">{{ saving ? '保存中…' : '保存资料' }}</button>
    </form>
  </div>
</template>

<style scoped>
h1 { font-size: 24px; margin-bottom: 20px; }
.site-form { display: grid; grid-template-columns: repeat(2,minmax(0,1fr)); gap: 16px; max-width: 820px; padding: 22px; background: var(--bg-raised); border: 1px solid var(--line); border-radius: var(--radius-lg); }
label { display: flex; flex-direction: column; gap: 6px; font-size: 13px; color: var(--ink-soft); }
.wide { grid-column: 1 / -1; }
input, textarea, select { width: 100%; padding: 9px 11px; border: 1px solid var(--line-strong); border-radius: var(--radius); font: inherit; color: var(--ink); background: #fff; }
textarea { resize: vertical; }
button { justify-self: start; padding: 10px 22px; border: 0; border-radius: var(--radius); color: #fff; background: var(--accent); cursor: pointer; }
button:disabled { opacity: .6; }
p[role=status] { align-self: center; }
.skill-row { display: grid; grid-template-columns: 180px 1fr auto; gap: 8px; align-items: start; }
.site-form strong { color: var(--ink); }
.site-form .wide p { font-size: 12px; margin-top: 5px; }
.section-head { margin-top: 8px; padding-top: 16px; border-top: 1px solid var(--line); }
.page-sections { display: grid; gap: 12px; }
.page-sections__head { display: flex; justify-content: space-between; align-items: center; }
.page-sections__head h2 { font-size: 17px; }
.module-editor { display: grid; gap: 12px; padding: 18px; border: 1px solid var(--line); border-radius: var(--radius); background: var(--bg); }
.module-editor__head { display: flex; justify-content: flex-end; align-items: center; flex-wrap: wrap; gap: 8px; }
.module-editor__head .check { margin-right: auto; display: flex; flex-direction: row; align-items: center; }
.module-editor__head input[type=checkbox] { width: auto; }
.module-editor__head button { padding: 6px 10px; }
.module-preview { padding: 14px; border: 1px dashed var(--line-strong); border-radius: var(--radius); background: var(--bg-raised); }
.module-preview > strong { font-size: 12px; color: var(--ink-faint); }
.module-preview > div { margin-top: 12px; line-height: 1.7; overflow-wrap: anywhere; }
.module-preview :deep(img) { max-width: 100%; }
.avatar-editor { display: grid; gap: 9px; font-size: 13px; color: var(--ink-soft); }
.avatar-editor__row { display: flex; align-items: center; flex-wrap: wrap; gap: 12px; }
.avatar-editor__row img, .avatar-editor__fallback { width: 76px; height: 76px; border-radius: var(--radius-lg); object-fit: cover; }
.avatar-editor__fallback { display: grid; place-items: center; background: var(--accent); color: #fff; font-size: 30px; }
.avatar-editor__upload { display: inline-block; padding: 9px 14px; border: 1px solid var(--line-strong); border-radius: var(--radius); color: var(--accent); cursor: pointer; }
.avatar-editor__remove { background: transparent; color: #b3351f; border: 1px solid var(--line-strong); padding: 9px 14px; }
.avatar-editor small { color: var(--ink-faint); }
.avatar-editor p[role=alert] { color: #b3351f; }
@media (max-width: 600px) { .site-form { grid-template-columns: 1fr; } }
</style>
