import MarkdownIt from 'markdown-it'
import hljs from 'highlight.js'
import type { TocItem } from '@/api/types'

/** 生成标题锚点 id */
function slugify(text: string): string {
  // 保留中文/英文/数字，其余转连字符
  const cleaned = text
    .trim()
    .toLowerCase()
    .replace(/[^\p{L}\p{N}]+/gu, '-')
    .replace(/^-+|-+$/g, '')
  return cleaned || `heading-${Math.abs(hash(text))}`
}

function hash(s: string): number {
  let h = 0
  for (let i = 0; i < s.length; i++) {
    h = (h << 5) - h + s.charCodeAt(i)
    h |= 0
  }
  return h
}

const md = new MarkdownIt({
  html: true,
  linkify: true,
  typographer: false,
  highlight(str: string, lang: string): string {
    const language = lang.trim()
    if (language && hljs.getLanguage(language)) {
      try {
        const highlighted = hljs.highlight(str, {
          language,
          ignoreIllegals: true,
        }).value
        return (
          `<pre class="code-block"><span class="code-block__lang">${md.utils.escapeHtml(language)}</span>` +
          `<code class="hljs language-${md.utils.escapeHtml(language)}">${highlighted}</code></pre>`
        )
      } catch {
        /* fall through to escaped */
      }
    }
    return (
      `<pre class="code-block"><code class="hljs">${md.utils.escapeHtml(str)}</code></pre>`
    )
  },
})

// 外链新窗口打开
const defaultLinkRender =
  md.renderer.rules.link_open ||
  ((tokens, idx, options, _env, self) => self.renderToken(tokens, idx, options))
md.renderer.rules.link_open = (tokens, idx, options, env, self) => {
  const href = String(tokens[idx].attrGet('href') ?? '')
  if (/^https?:\/\//.test(href)) {
    tokens[idx].attrSet('target', '_blank')
    tokens[idx].attrSet('rel', 'noopener noreferrer')
  }
  return defaultLinkRender(tokens, idx, options, env, self)
}

export interface RenderResult {
  html: string
  headings: TocItem[]
}

// 模块级收集器：规则只注册一次，渲染前重置
let collected: TocItem[] = []
let usedIds = new Set<string>()

md.core.ruler.push('extract_headings', (state) => {
  state.tokens.forEach((token, i) => {
    if (token.type !== 'heading_open') return
    const level = Number(token.tag.slice(1))
    const inline = state.tokens[i + 1]
    const text = inline && inline.type === 'inline' ? inline.content : ''
    let id = slugify(text)
    let n = 1
    const base = id
    while (usedIds.has(id)) {
      id = `${base}-${n++}`
    }
    usedIds.add(id)
    token.attrSet('id', id)
    if (level >= 2 && level <= 3) {
      collected.push({ id, text, level })
    }
  })
})

/** 渲染 Markdown，同时提取 h2/h3 作为目录 */
export function renderMarkdown(content: string): RenderResult {
  collected = []
  usedIds = new Set<string>()
  const html = md.render(content)
  return { html, headings: collected }
}

/** 去掉 markdown 语法，用于摘要/搜索 */
export function stripMarkdown(content: string): string {
  return content
    .replace(/```[\s\S]*?```/g, ' ')
    .replace(/`([^`]+)`/g, '$1')
    .replace(/!\[[^\]]*\]\([^)]*\)/g, ' ')
    .replace(/\[([^\]]*)\]\([^)]*\)/g, '$1')
    .replace(/^#{1,6}\s+/gm, '')
    .replace(/[*_>~#-]/g, '')
    .replace(/\s+/g, ' ')
    .trim()
}

/* ---------------------------------------------------------------------------
 * 评论专用渲染器：html 必须关闭。
 * 评论来自任意访客，若允许内嵌 HTML，会在 v-html 时形成存储型 XSS。
 * --------------------------------------------------------------------------- */
const mdComment = new MarkdownIt({
  html: false,
  linkify: true,
  typographer: false,
})

// 评论里的外链同样新窗口打开
const commentLinkRender =
  mdComment.renderer.rules.link_open ||
  ((tokens, idx, options, _env, self) => self.renderToken(tokens, idx, options))
mdComment.renderer.rules.link_open = (tokens, idx, options, env, self) => {
  const href = String(tokens[idx].attrGet('href') ?? '')
  if (/^https?:\/\//.test(href)) {
    tokens[idx].attrSet('target', '_blank')
    tokens[idx].attrSet('rel', 'noopener noreferrer nofollow')
  }
  return commentLinkRender(tokens, idx, options, env, self)
}

/** 渲染访客评论（Markdown -> HTML，禁止内嵌 HTML，防 XSS） */
export function renderComment(content: string): string {
  return mdComment.render(content)
}
