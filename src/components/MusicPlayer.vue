<script setup lang="ts">
import { computed, nextTick, onMounted, ref, watch } from 'vue'
import { blogApi } from '@/api'
import { useRoute } from 'vue-router'
import type { MusicTrack } from '@/api/types'
import { articleMusicId } from '@/utils/musicSelection'

type Mode = 'sequence' | 'single'
const tracks = ref<MusicTrack[]>([])
const route = useRoute()
const currentId = ref<number | null>(null)
const current = computed(() => tracks.value.find(track => track.id === currentId.value) ?? null)
const audio = ref<HTMLAudioElement | null>(null)
const playing = ref(false)
const playBlocked = ref(false)
const homeEnabled = ref(true)
const homeMusicId = ref(0)
const showPlayer = computed(() => route.path !== '/' || homeEnabled.value)
const expanded = ref(false)
const collapsed = ref(localStorage.getItem('blog_music_collapsed') === 'true')
const elapsed = ref(0)
const duration = ref(0)
const mode = ref<Mode>(localStorage.getItem('blog_music_mode') === 'single' ? 'single' : 'sequence')
let transitionId = 0

onMounted(async () => {
  const [musicResult, siteResult] = await Promise.allSettled([blogApi.listMusic(), blogApi.getSite()])
  if (musicResult.status !== 'fulfilled') return
  tracks.value = musicResult.value
  if (siteResult.status === 'fulfilled') {
    homeEnabled.value = siteResult.value.homeMusicEnabled !== false
    homeMusicId.value = siteResult.value.homeMusicId ?? 0
  }
  const homeTrackId = tracks.value.find(track => track.id === homeMusicId.value)?.id ?? tracks.value[0]?.id ?? null
  currentId.value = (route.name === 'post' ? tracks.value.find(track => track.id === articleMusicId.value)?.id : null) ?? homeTrackId
  if (route.name === 'post' && articleMusicId.value && currentId.value === articleMusicId.value) { await nextTick(); void play() }
})
watch(articleMusicId, async id => {
  if (route.name !== 'post' || !id || !tracks.value.some(track => track.id === id)) return
  await switchTo(id, true)
})
watch(() => route.path, async path => {
  if (path !== '/' || !tracks.value.length) return
  if (!homeEnabled.value) {
    ++transitionId
    audio.value?.pause()
    return
  }
  const id = tracks.value.find(track => track.id === homeMusicId.value)?.id ?? tracks.value[0].id
  await switchTo(id, false)
})
watch(mode, value => localStorage.setItem('blog_music_mode', value))
watch(collapsed, value => localStorage.setItem('blog_music_collapsed', String(value)))

async function fade(to: number, milliseconds: number, token: number) {
  const element = audio.value
  if (!element) return
  const from = element.volume
  const started = performance.now()
  while (token === transitionId) {
    const fraction = Math.min(1, (performance.now() - started) / milliseconds)
    element.volume = from + (to - from) * fraction
    if (fraction === 1) break
    await new Promise(resolve => setTimeout(resolve, 20))
  }
}

async function switchTo(id: number, startIfPaused: boolean) {
  if (currentId.value === id) {
    if (startIfPaused && !playing.value) await play()
    return
  }
  const token = ++transitionId
  const shouldPlay = playing.value || startIfPaused
  if (playing.value) await fade(0, 280, token)
  if (token !== transitionId) return
  audio.value?.pause()
  currentId.value = id
  elapsed.value = 0
  duration.value = 0
  await nextTick()
  if (token !== transitionId || !audio.value) return
  audio.value.volume = 0
  audio.value.load()
  if (shouldPlay) await play()
  if (token !== transitionId) return
  if (playing.value) await fade(1, 420, token)
  else audio.value.volume = 1
}

async function play() {
  if (!audio.value) return
  try { await audio.value.play(); playing.value = true; playBlocked.value = false }
  catch { playing.value = false; playBlocked.value = true }
}
function toggle() {
  if (!audio.value) return
  if (playing.value) { audio.value.pause(); playing.value = false }
  else void play()
}
async function skip(direction: number) {
  if (!tracks.value.length) return
  const index = tracks.value.findIndex(track => track.id === currentId.value)
  await switchTo(tracks.value[(index + direction + tracks.value.length) % tracks.value.length].id, true)
}
async function choose(id: number) {
  expanded.value = false
  await switchTo(id, true)
}
function onEnded() { playing.value = false; if (mode.value === 'sequence') void skip(1) }
function seek(event: Event) {
  if (!audio.value) return
  audio.value.currentTime = Number((event.target as HTMLInputElement).value)
}
function time(seconds: number) {
  if (!Number.isFinite(seconds)) return '0:00'
  return `${Math.floor(seconds / 60)}:${String(Math.floor(seconds % 60)).padStart(2, '0')}`
}
</script>

<template>
  <aside v-if="showPlayer && tracks.length && current" class="music-player" :class="{ 'music-player--collapsed': collapsed }" aria-label="音乐播放器">
    <audio :key="current.id" ref="audio" :src="current.url" :loop="mode === 'single'" preload="metadata" @ended="onEnded" @timeupdate="elapsed = audio?.currentTime || 0" @loadedmetadata="duration = audio?.duration || 0" @pause="playing = false" @play="playing = true"></audio>
    <button v-if="collapsed" type="button" class="collapsed-trigger" :title="`展开音乐播放器：${current.title}`" aria-label="展开音乐播放器" @click="collapsed = false"><span>♫</span><span v-if="playing" class="playing-dot"></span></button>
    <template v-else>
    <div class="music-head"><span class="music-icon">♫</span><div class="music-title"><strong>{{ current.title }}</strong><small>{{ current.artist || '博客音乐' }}</small></div><button type="button" :aria-label="expanded ? '收起曲目列表' : '展开曲目列表'" @click="expanded = !expanded">{{ expanded ? '⌄' : '☷' }}</button><button type="button" class="collapse-button" aria-label="收起音乐播放器" title="收起播放器" @click="collapsed = true; expanded = false">×</button></div>
    <div class="controls"><button type="button" aria-label="上一首" @click="skip(-1)">⏮</button><button type="button" class="play" :aria-label="playing ? '暂停' : '播放'" @click="toggle">{{ playing ? 'Ⅱ' : '▶' }}</button><button type="button" aria-label="下一首" @click="skip(1)">⏭</button><button type="button" class="mode" :title="mode === 'single' ? '单曲循环，点击切换为顺序播放' : '顺序播放，点击切换为单曲循环'" @click="mode = mode === 'single' ? 'sequence' : 'single'">{{ mode === 'single' ? '单曲循环' : '顺序播放' }}</button></div>
    <div class="progress"><span>{{ time(elapsed) }}</span><input type="range" min="0" :max="duration || 0" :value="elapsed" aria-label="播放进度" @input="seek" /><span>{{ time(duration) }}</span></div>
    <p v-if="playBlocked" class="play-hint">浏览器已暂停自动播放，点击播放继续。</p>
    <div v-if="expanded" class="playlist"><button v-for="track in tracks" :key="track.id" type="button" :class="{ active: track.id === currentId }" @click="choose(track.id)">{{ track.title }} <small>{{ track.artist }}</small></button></div>
    </template>
  </aside>
</template>

<style scoped>
.music-player { position: fixed; right: 18px; bottom: 145px; z-index: 60; width: min(330px, calc(100vw - 36px)); padding: 12px 14px; border: 1px solid var(--line-strong); border-radius: var(--radius-lg); background: var(--bg-raised); box-shadow: var(--shadow-card); }
.music-player--collapsed { width: auto; padding: 0; border-radius: 50%; }
.collapsed-trigger { display: grid; place-items: center; position: relative; width: 48px; height: 48px; border-radius: 50%; background: var(--accent); color: white; font-size: 25px; }
.playing-dot { position: absolute; right: 7px; top: 7px; width: 7px; height: 7px; border-radius: 50%; background: #8dc875; }
.collapse-button { font-size: 20px; line-height: 1; }
.music-head { display: flex; align-items: center; gap: 10px; }
.music-icon { display: grid; place-items: center; width: 34px; height: 34px; border-radius: var(--radius); color: white; background: var(--accent); }
.music-title { display: grid; min-width: 0; flex: 1; }
.music-title strong { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; font-size: 13px; }
.music-title small { color: var(--ink-faint); font-size: 11px; }
button { border: 0; background: none; color: var(--ink-soft); cursor: pointer; }
.controls { display: flex; align-items: center; gap: 12px; margin-top: 10px; }
.controls button { font-size: 15px; }
.controls .play { display: grid; place-items: center; width: 30px; height: 30px; border-radius: 50%; background: var(--accent); color: white; font-size: 12px; }
.controls .mode { margin-left: auto; padding: 5px 7px; border-radius: var(--radius); background: var(--bg-inset); color: var(--accent); font-size: 11px; }
.progress { display: flex; align-items: center; gap: 7px; margin-top: 8px; color: var(--ink-faint); font: 10px var(--font-mono); }
.progress input { flex: 1; min-width: 0; accent-color: var(--accent); }
.play-hint { margin-top: 7px; color: var(--amber); font-size: 11px; }
.playlist { display: grid; max-height: 180px; overflow-y: auto; margin-top: 10px; padding-top: 8px; border-top: 1px solid var(--line); }
.playlist button { display: flex; justify-content: space-between; gap: 8px; padding: 7px; text-align: left; font-size: 12px; }
.playlist button.active { color: var(--accent); background: var(--accent-soft); }
.playlist small { color: var(--ink-faint); }
@media(max-width:600px) { .music-player { right: 8px; top: auto; bottom: 72px; width: min(330px, calc(100vw - 16px)); } .music-player--collapsed { width: auto; } }
</style>
