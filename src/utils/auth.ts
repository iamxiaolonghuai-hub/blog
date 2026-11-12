export const TOKEN_KEY = 'blog_token'
export const READER_TOKEN_KEY = 'blog_reader_token'
export const READER_NAME_KEY = 'blog_reader_name'
export function getReaderToken(): string | null { return localStorage.getItem(READER_TOKEN_KEY) }
export function getReaderName(): string | null { return localStorage.getItem(READER_NAME_KEY) }
export function setReaderSession(token: string, username: string): void {
  localStorage.setItem(READER_TOKEN_KEY, token)
  localStorage.setItem(READER_NAME_KEY, username)
  window.dispatchEvent(new Event('blog-reader-session'))
}
export function clearReaderSession(): void {
  localStorage.removeItem(READER_TOKEN_KEY)
  localStorage.removeItem(READER_NAME_KEY)
}

export function getToken(): string | null {
  return localStorage.getItem(TOKEN_KEY)
}

export function setToken(token: string): void {
  localStorage.setItem(TOKEN_KEY, token)
}

export function clearToken(): void {
  localStorage.removeItem(TOKEN_KEY)
}

export function isLoggedIn(): boolean {
  return !!getToken()
}
