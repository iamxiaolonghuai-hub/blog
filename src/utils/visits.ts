import { post } from '@/api/request'
import { getToken } from '@/utils/auth'

const VISITOR_KEY = 'blog_visitor_id'
let fallbackId = ''

function visitorId(): string {
  try {
    const saved = localStorage.getItem(VISITOR_KEY)
    if (saved) return saved
    const bytes = new Uint8Array(16)
    crypto.getRandomValues(bytes)
    const id = `v-${Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join('')}`
    localStorage.setItem(VISITOR_KEY, id)
    return id
  } catch {
    fallbackId ||= `v-${Date.now()}-${Math.random().toString(36).slice(2)}`
    return fallbackId
  }
}

export function trackVisit(path: string): void {
  if (path.startsWith('/admin') || getToken()) return
  void post('/visits', { visitorId: visitorId(), path }).catch(() => {})
}
