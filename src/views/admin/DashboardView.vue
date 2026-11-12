<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { adminApi } from '@/api/admin'
import type { AdminStats, AnalyticsPeriod, AnalyticsPoint, Comment, Post } from '@/api/types'
import { formatDate } from '@/utils/date'

const stats = ref<AdminStats | null>(null)
const pendingComments = ref<Comment[]>([])
const recentPosts = ref<Post[]>([])
const loading = ref(true)
const loadError = ref('')
const period = ref<AnalyticsPeriod>('week')
const trend = ref<AnalyticsPoint[]>([])
const trendError = ref('')
const trendLoading = ref(false)
const hoverIndex = ref<number | null>(null)

async function loadTrend(next: AnalyticsPeriod) {
  period.value = next
  trendLoading.value = true
  trendError.value = ''
  try { trend.value = (await adminApi.analytics(next)).points }
  catch { trendError.value = '趋势数据加载失败' }
  finally { trendLoading.value = false }
}

const chartRows = computed(() => {
  const now = Date.now() + 8 * 3600000
  const count = period.value === 'day' ? 24 : period.value === 'week' ? 7 : period.value === 'month' ? 30 : 12
  const values = new Map(trend.value.map(point => [point.bucket, point]))
  return Array.from({ length: count }, (_, index) => {
    const back = count - 1 - index
    let bucket: string
    if (period.value === 'year') {
      const china = new Date(now)
      bucket = new Date(Date.UTC(china.getUTCFullYear(), china.getUTCMonth() - back, 1)).toISOString().slice(0, 7)
    } else {
      const offset = back * (period.value === 'day' ? 3600000 : 86400000)
      const date = new Date(now - offset).toISOString()
      bucket = period.value === 'day' ? date.slice(0, 13).replace('T', ' ') : date.slice(0, 10)
    }
    return values.get(bucket) ?? { bucket, views: 0, guests: 0, readers: 0, newAccounts: 0 }
  })
})
const chartMax = computed(() => Math.max(1, ...chartRows.value.flatMap(row => [row.views, row.guests, row.readers, row.newAccounts])))
const currentPoint = computed(() => chartRows.value[hoverIndex.value ?? chartRows.value.length - 1])
function linePoints(metric: 'views' | 'guests' | 'readers' | 'newAccounts') {
  return chartRows.value.map((row, index) => `${52 + index * 716 / Math.max(1, chartRows.value.length - 1)},${168 - row[metric] * 130 / chartMax.value}`).join(' ')
}
function shortLabel(bucket: string) { return period.value === 'day' ? `${bucket.slice(-2)}时` : period.value === 'year' ? bucket.slice(5) + '月' : bucket.slice(5) }

onMounted(async () => {
  void loadTrend('week')
  try {
  const [s, pending, posts] = await Promise.all([
    adminApi.stats(),
    adminApi.listComments('pending'),
    adminApi.listAllPosts(),
  ])
  stats.value = s
  pendingComments.value = pending.slice(0, 4)
  recentPosts.value = posts.slice(0, 5)
  } catch { loadError.value = '后台数据加载失败，请检查后端服务后刷新页面' }
  finally { loading.value = false }
})

interface CardDef {
  key: keyof AdminStats
  label: string
  symbol: string
  highlight?: boolean
}

const cards: CardDef[] = [
  { key: 'postCount', label: '已发布文章', symbol: '◧' },
  { key: 'commentCount', label: '评论总数', symbol: '◩' },
  { key: 'pendingCommentCount', label: '待审评论', symbol: '!', highlight: true },
  { key: 'linkCount', label: '友情链接', symbol: '◪' },
  { key: 'totalViews', label: '文章浏览量（历史累计）', symbol: '◉' },
  { key: 'siteViews', label: '全站浏览量', symbol: '↗' },
  { key: 'guestVisitors', label: '游客人数', symbol: '◌' },
  { key: 'registeredVisitors', label: '已访问的注册用户', symbol: '◍' },
  { key: 'readerAccounts', label: '注册账号总数', symbol: '▣' },
]
</script>

<template>
  <div class="dash">
    <div class="dash__head">
      <h1>仪表盘</h1>
      <p class="dash__greet">欢迎回来，这里是站点的整体概况。</p>
    </div>

    <div v-if="loading" class="loading"><span class="loading__dot"></span> 加载中…</div>
    <p v-else-if="loadError" role="alert">{{ loadError }}</p>

    <template v-else-if="stats">
      <div class="stat-grid">
        <div
          v-for="c in cards"
          :key="c.key"
          class="stat-card"
          :class="{ 'stat-card--hot': c.highlight && stats[c.key] > 0 }"
        >
          <span class="stat-card__symbol">{{ c.symbol }}</span>
          <span class="stat-card__value">{{ stats[c.key] }}</span>
          <span class="stat-card__label">{{ c.label }}</span>
        </div>
      </div>

      <section class="trend-panel">
        <div class="trend-head"><div><h2>访问趋势</h2><p>全站浏览量与独立访客 · 北京时间 · 从启用统计后开始记录</p></div>
          <div class="period-tabs"><button v-for="option in ([['day','日'],['week','周'],['month','月'],['year','年']] as const)" :key="option[0]" type="button" :class="{ active: period === option[0] }" @click="loadTrend(option[0])">{{ option[1] }}</button></div>
        </div>
        <p v-if="trendError" role="alert">{{ trendError }}</p>
        <div v-else class="chart-wrap" :class="{ 'is-loading': trendLoading }">
          <svg class="chart" viewBox="0 0 800 215" role="img" aria-label="访问趋势折线图">
            <g v-for="step in 5" :key="step"><line x1="52" :y1="168 - (step - 1) * 32.5" x2="768" :y2="168 - (step - 1) * 32.5" class="grid-line" /><text x="6" :y="172 - (step - 1) * 32.5" class="axis-text">{{ Math.round(chartMax * (step - 1) / 4) }}</text></g>
            <polyline :points="linePoints('views')" class="line line--views" />
            <polyline :points="linePoints('guests')" class="line line--guests" />
            <polyline :points="linePoints('readers')" class="line line--readers" />
            <polyline :points="linePoints('newAccounts')" class="line line--accounts" />
            <g v-for="(row, index) in chartRows" :key="row.bucket">
              <rect :x="52 + index * 716 / Math.max(1, chartRows.length - 1) - 10" y="30" width="20" height="158" class="hit" @mouseenter="hoverIndex = index" @focus="hoverIndex = index" tabindex="0"><title>{{ row.bucket }}：浏览 {{ row.views }}，游客 {{ row.guests }}，注册用户 {{ row.readers }}，新注册 {{ row.newAccounts }}</title></rect>
              <text v-if="index === 0 || index === chartRows.length - 1 || index % Math.max(1, Math.floor(chartRows.length / 6)) === 0" :x="52 + index * 716 / Math.max(1, chartRows.length - 1)" y="203" text-anchor="middle" class="axis-text">{{ shortLabel(row.bucket) }}</text>
            </g>
          </svg>
          <div class="chart-legend"><span><i class="legend-views"></i> 浏览量</span><span><i class="legend-guests"></i> 游客</span><span><i class="legend-readers"></i> 注册用户</span><span><i class="legend-accounts"></i> 新注册</span></div>
          <p v-if="currentPoint" class="chart-detail">{{ currentPoint.bucket }}　浏览 {{ currentPoint.views }} · 游客 {{ currentPoint.guests }} · 注册用户 {{ currentPoint.readers }} · 新注册 {{ currentPoint.newAccounts }}</p>
        </div>
      </section>

      <div class="dash__cols">
        <section class="panel">
          <div class="panel__head">
            <h2>待审评论</h2>
            <router-link to="/admin/comments" class="panel__more">全部 →</router-link>
          </div>
          <p v-if="!pendingComments.length" class="panel__empty">没有待审评论</p>
          <ul v-else class="mini-list">
            <li v-for="c in pendingComments" :key="c.id">
              <div class="mini-comment">
                <span class="mini-comment__author">{{ c.author }}</span>
                <span class="mini-comment__post">《{{ c.postTitle }}》</span>
                <p class="mini-comment__text">{{ c.content }}</p>
                <span class="mini-comment__date">{{ formatDate(c.createdAt) }}</span>
              </div>
            </li>
          </ul>
        </section>

        <section class="panel">
          <div class="panel__head">
            <h2>最近文章</h2>
            <router-link to="/admin/posts" class="panel__more">管理 →</router-link>
          </div>
          <ul class="mini-posts">
            <li v-for="p in recentPosts" :key="p.id">
              <router-link :to="`/admin/posts/${p.id}/edit`" class="mini-post">
                <span class="mini-post__title">{{ p.title }}</span>
                <span class="mini-post__meta">
                  <span class="mini-post__status" :class="`is-${p.status}`">{{ p.status === 'published' ? '已发布' : '草稿' }}</span>
                  {{ formatDate(p.updatedAt) }}
                </span>
              </router-link>
            </li>
          </ul>
        </section>
      </div>
    </template>
  </div>
</template>

<style scoped>
.dash__head { margin-bottom: 26px; }
.dash__head h1 { font-size: 26px; margin-bottom: 6px; }
.dash__greet { font-size: 14px; color: var(--ink-faint); }

.stat-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 14px;
  margin-bottom: 30px;
}
.stat-card {
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  padding: 18px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}
.stat-card--hot { border-color: var(--amber); background: var(--amber-soft); }
.stat-card__symbol {
  font-family: var(--font-mono);
  color: var(--amber);
  font-size: 14px;
}
.stat-card__value {
  font-size: 30px;
  font-weight: 600;
  line-height: 1.1;
}
.stat-card__label { font-size: 13px; color: var(--ink-faint); }
.trend-panel { margin-bottom: 30px; padding: 20px; border: 1px solid var(--line); border-radius: var(--radius-lg); background: var(--bg-raised); }
.trend-head { display: flex; justify-content: space-between; align-items: start; gap: 16px; flex-wrap: wrap; }
.trend-head h2 { font-size: 17px; }
.trend-head p { margin-top: 5px; color: var(--ink-faint); font-size: 12px; }
.period-tabs { display: flex; gap: 4px; padding: 3px; border-radius: var(--radius); background: var(--bg-inset); }
.period-tabs button { padding: 6px 12px; border-radius: var(--radius); background: transparent; color: var(--ink-soft); font-size: 13px; }
.period-tabs button.active { color: white; background: var(--accent); }
.chart-wrap { margin-top: 18px; }
.chart-wrap.is-loading { opacity: .5; }
.chart { width: 100%; height: auto; max-height: 260px; }
.grid-line { stroke: var(--line); stroke-width: 1; }
.axis-text { fill: var(--ink-faint); font-size: 11px; font-family: var(--font-mono); }
.line { fill: none; stroke-width: 2.5; stroke-linecap: round; stroke-linejoin: round; }
.line--views { stroke: var(--accent); }
.line--guests { stroke: var(--amber); }
.line--readers { stroke: #5b9878; }
.line--accounts { stroke: #956ab0; }
.hit { fill: transparent; cursor: crosshair; }
.chart-legend { display: flex; gap: 18px; flex-wrap: wrap; font-size: 12px; color: var(--ink-soft); }
.chart-legend span { display: inline-flex; align-items: center; gap: 6px; }
.chart-legend i { display: inline-block; width: 15px; height: 3px; border-radius: 2px; }
.legend-views { background: var(--accent); } .legend-guests { background: var(--amber); } .legend-readers { background: #5b9878; } .legend-accounts { background: #956ab0; }
.chart-detail { margin-top: 10px; font-family: var(--font-mono); font-size: 12px; color: var(--ink-soft); }

.dash__cols {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 18px;
}
.panel {
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  padding: 20px;
}
.panel__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 14px;
}
.panel__head h2 { font-size: 16px; }
.panel__more { font-size: 13px; }
.panel__empty {
  font-size: 13px;
  color: var(--ink-faint);
  padding: 10px 0;
}
.mini-list { list-style: none; margin: 0; padding: 0; }
.mini-comment {
  padding: 10px 0;
  border-top: 1px dashed var(--line);
}
.mini-comment__author { font-weight: 600; font-size: 14px; }
.mini-comment__post { font-size: 12px; color: var(--ink-faint); margin-left: 6px; }
.mini-comment__text {
  font-size: 13.5px;
  color: var(--ink-soft);
  margin: 5px 0;
}
.mini-comment__date {
  font-family: var(--font-mono);
  font-size: 11.5px;
  color: var(--ink-faint);
}
.mini-posts { list-style: none; margin: 0; padding: 0; }
.mini-post {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 10px 0;
  border-top: 1px dashed var(--line);
  color: var(--ink);
}
.mini-post__title { font-size: 13.5px; }
.mini-post__meta {
  font-family: var(--font-mono);
  font-size: 11px;
  color: var(--ink-faint);
  white-space: nowrap;
  display: flex;
  align-items: center;
  gap: 6px;
}
.mini-post__status {
  padding: 1px 6px;
  border-radius: 3px;
}
.mini-post__status.is-published { background: var(--accent-soft); color: var(--accent); }
.mini-post__status.is-draft { background: var(--bg-inset); color: var(--ink-faint); }

@media (max-width: 860px) {
  .dash__cols { grid-template-columns: 1fr; }
}
</style>
