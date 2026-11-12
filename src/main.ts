import { createApp } from 'vue'

// 本地字体（@fontsource，构建进产物，不依赖 CDN）
import '@fontsource/space-grotesk/400.css'
import '@fontsource/space-grotesk/500.css'
import '@fontsource/space-grotesk/600.css'
import '@fontsource/newsreader/400.css'
import '@fontsource/newsreader/400-italic.css'
import '@fontsource/newsreader/700.css'
import '@fontsource/jetbrains-mono/400.css'
import '@fontsource/jetbrains-mono/500.css'
import '@fontsource/jetbrains-mono/600.css'

import '@/styles/main.css'
import '@/styles/markdown.css'

import App from './App.vue'
import router from './router'

createApp(App).use(router).mount('#app')
