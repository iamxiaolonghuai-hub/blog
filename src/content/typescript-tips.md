TypeScript 的价值不在于把每个变量都标上类型，而在于让**数据结构的约束在编译期就被检查**。这篇整理几个在 Vue 项目里高频用到的类型技巧。

## 给接口响应定义类型

后端返回的 JSON 结构应该和前端的 interface 一一对应：

```ts
export interface Post {
  id: number
  slug: string
  title: string
  tags: string[]
  createdAt: string
}
```

配合 axios 封装，请求方法就能返回带类型的 Promise：

```ts
async function getPost(slug: string): Promise<Post> {
  const { data } = await http.get<Post>(`/posts/${slug}`)
  return data
}
```

## 用联合类型收敛状态

不要用一堆布尔值表示状态，用可辨识联合更安全：

```ts
type RequestState<T> =
  | { status: 'loading' }
  | { status: 'success'; data: T }
  | { status: 'error'; message: string }
```

在模板里判断 `status` 时，编译器会确保你处理了所有分支，漏写一个就报错。

## typeof 与 keyof 复用已有结构

```ts
const tagColors = {
  vue: '#42b883',
  cpp: '#1f3a5f',
  ts: '#3178c6'
} as const

type TagName = keyof typeof tagColors
```

`as const` 让对象变成只读的字面量类型，`keyof` 直接提取出所有合法的键，新增标签时类型自动更新。

## 泛型工具函数

```ts
function paginate<T>(list: T[], page: number, size: number): T[] {
  return list.slice((page - 1) * size, page * size)
}
```

泛型让函数既能复用逻辑，又不丢失具体的元素类型——`paginate(posts, 1, 10)` 的返回值依然是 `Post[]`。

## Vue 组件里的类型

```ts
import { ref } from 'vue'

const posts = ref<Post[]>([])
const activeTag = ref<string | null>(null)
```

`ref<Post[]>` 让模板和脚本里的 `posts.value` 都有完整的类型提示，这是用 TS 写 Vue 最直接的收益。

> 类型是用来服务开发的，别为了消除报错随手 `any`——那等于把 TypeScript 又写回了 JavaScript。

掌握这几个模式，日常项目里绝大多数类型问题都能覆盖。
