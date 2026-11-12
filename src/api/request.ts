import axios, { type AxiosProgressEvent } from 'axios'
import { clearReaderSession, clearToken, getReaderToken, getToken } from '@/utils/auth'

/**
 * 统一 axios 实例。
 * baseURL 默认走 /api，由 Vite 代理到 Drogon；
 * 生产环境可通过 VITE_API_BASE 指向真实后端。
 */
export const http = axios.create({
  baseURL: import.meta.env.VITE_API_BASE || '/api',
  timeout: 10000,
})

// 请求拦截：可在此注入 JWT
http.interceptors.request.use((config) => {
  const readerRequest = config.url?.includes('/posts/') && config.url?.endsWith('/comments') && config.method === 'post'
    || config.url === '/upload/comment' || config.url === '/visits' || /^\/resources\/\d+\/access$/.test(config.url || '')
  const token = readerRequest ? (getReaderToken() || getToken()) : getToken()
  if (token) {
    config.headers.Authorization = `Bearer ${token}`
  }
  return config
})

// 响应拦截：直接取 data，业务层拿到的就是后端 JSON
http.interceptors.response.use(
  (response) => response.data,
  (error) => {
    if (error.response?.status === 401) {
      const url = error.config?.url as string | undefined
      if (url?.includes('/posts/') || url === '/upload/comment' || /^\/resources\/\d+\/access$/.test(url || '')) {
        clearReaderSession()
      } else if (url?.startsWith('/admin/')) {
        clearToken()
        if (!window.location.pathname.startsWith('/admin/login')) {
          window.location.href = '/admin/login'
        }
      }
    }
    return Promise.reject(error)
  },
)

/** 类型安全的 GET（拦截器已剥离 AxiosResponse） */
export function get<T>(url: string, params?: Record<string, unknown>): Promise<T> {
  return http.get(url, { params }) as unknown as Promise<T>
}

/** 类型安全的 POST */
export function post<T>(url: string, body?: unknown): Promise<T> {
  return http.post(url, body) as unknown as Promise<T>
}

/** 文件上传（multipart/form-data），返回后端 JSON（通常含 url） */
export function upload<T>(url: string, file: File | Blob, field = 'file', onProgress?: (event: AxiosProgressEvent) => void): Promise<T> {
  const form = new FormData()
  form.append(field, file)
  // 不手动设置 Content-Type，让浏览器自动带上 boundary；
  // 上传耗时更长，单独放宽超时
  return http.post(url, form, { timeout: 600000, onUploadProgress: onProgress }) as unknown as Promise<T>
}

/** 类型安全的 PUT */
export function put<T>(url: string, body?: unknown): Promise<T> {
  return http.put(url, body) as unknown as Promise<T>
}

/** 类型安全的 DELETE */
export function del<T>(url: string): Promise<T> {
  return http.delete(url) as unknown as Promise<T>
}
