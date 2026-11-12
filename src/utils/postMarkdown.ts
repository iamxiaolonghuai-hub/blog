import type { Post, PostInput } from '@/api/types'

export function parsePostMarkdown(source: string, filename: string): Partial<PostInput> {
  const normalized = source.replace(/^\uFEFF/, '').replace(/\r\n/g, '\n')
  let content = normalized
  const metadata: Record<string, unknown> = {}
  if (normalized.startsWith('---\n')) {
    const end = normalized.indexOf('\n---\n', 4)
    if (end < 0) throw new Error('Markdown 元信息缺少结束分隔符 ---')
    for (const line of normalized.slice(4, end).split('\n')) {
      const match = line.match(/^([\w]+):\s*(.*)$/)
      if (!match) continue
      const [, key, raw] = match
      try { metadata[key] = JSON.parse(raw) } catch { metadata[key] = raw.replace(/^['"]|['"]$/g, '') }
    }
    content = normalized.slice(end + 5).trimStart()
  }
  if (!content.trim()) throw new Error('Markdown 正文不能为空')
  const string = (key: string) => typeof metadata[key] === 'string' ? metadata[key] as string : ''
  const tags = Array.isArray(metadata.tags) ? metadata.tags.filter((tag): tag is string => typeof tag === 'string') : string('tags').split(/[,，]/).map(tag => tag.trim()).filter(Boolean)
  return {
    title: string('title') || content.match(/^#\s+(.+)$/m)?.[1]?.trim() || filename.replace(/\.(md|markdown)$/i, ''),
    slug: string('slug'), summary: string('summary'), category: string('category'),
    cover: string('cover'), tags, status: metadata.status === 'published' ? 'published' : 'draft', content,
    isPinned: metadata.isPinned === true, isHot: metadata.isHot === true,
    displayWeight: Number.isInteger(metadata.displayWeight) ? Number(metadata.displayWeight) : 0,
  }
}

export function serializePostMarkdown(post: Post): string {
  const fields = ['title', 'slug', 'summary', 'category', 'cover', 'tags', 'status', 'isPinned', 'isHot', 'displayWeight', 'createdAt', 'updatedAt'] as const
  return `---\n${fields.map(key => `${key}: ${JSON.stringify(post[key] ?? '')}`).join('\n')}\n---\n\n${post.content.trimEnd()}\n`
}

export function downloadText(name: string, content: string, type = 'text/markdown;charset=utf-8') {
  const url = URL.createObjectURL(new Blob([content], { type }))
  const link = document.createElement('a')
  link.href = url
  link.download = name
  link.click()
  setTimeout(() => URL.revokeObjectURL(url), 1000)
}
