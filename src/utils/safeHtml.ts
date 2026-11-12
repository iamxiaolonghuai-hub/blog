import DOMPurify from 'dompurify'

/** 后台可写的内容模块只允许常用排版标签，不执行脚本或内联样式。 */
export function renderSafeHtml(html: string): string {
  const clean = DOMPurify.sanitize(html, {
    ALLOWED_TAGS: ['h2', 'h3', 'h4', 'p', 'br', 'strong', 'b', 'em', 'i', 'u', 's', 'a', 'ul', 'ol', 'li', 'blockquote', 'pre', 'code', 'hr', 'img', 'table', 'thead', 'tbody', 'tr', 'th', 'td', 'div', 'span'],
    ALLOWED_ATTR: ['href', 'src', 'alt', 'title', 'target', 'rel'],
  })
  const container = document.createElement('div')
  container.innerHTML = clean
  container.querySelectorAll('a[target="_blank"]').forEach(link => link.setAttribute('rel', 'noopener noreferrer'))
  return container.innerHTML
}
