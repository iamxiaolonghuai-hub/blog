/// <reference types="vite/client" />

interface ImportMetaEnv {
  /** 后端接口地址，默认 /api */
  readonly VITE_API_BASE?: string
  /** 是否强制使用本地 mock（'true' 时不请求后端） */
  readonly VITE_USE_MOCK?: string
  /** dev 代理目标，默认 http://localhost:8080 */
  readonly VITE_PROXY_TARGET?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}

declare module '*.md' {
  const content: string
  export default content
}
