import { ref } from 'vue'

export const articleMusicId = ref<number | null>(null)

export function selectArticleMusic(id: number | null | undefined): void {
  articleMusicId.value = id && id > 0 ? id : null
}
