import { createRouter, createWebHistory } from 'vue-router'
import { isLoggedIn } from '@/utils/auth'
import { trackVisit } from '@/utils/visits'

const routes = [
  {
    path: '/',
    name: 'home',
    component: () => import('@/views/HomeView.vue'),
  },
  {
    path: '/posts',
    name: 'posts',
    component: () => import('@/views/PostsView.vue'),
  },
  {
    path: '/posts/:slug',
    name: 'post',
    component: () => import('@/views/PostView.vue'),
  },
  {
    path: '/tags',
    name: 'tags',
    component: () => import('@/views/TagsView.vue'),
  },
  {
    path: '/tags/:tag',
    name: 'tag',
    component: () => import('@/views/TagsView.vue'),
  },
  {
    path: '/archive',
    name: 'archive',
    component: () => import('@/views/ArchiveView.vue'),
  },
  {
    path: '/links',
    name: 'links',
    component: () => import('@/views/LinksView.vue'),
  },
  {
    path: '/about',
    name: 'about',
    component: () => import('@/views/AboutView.vue'),
  },
  { path: '/resources', name: 'resources', component: () => import('@/views/ResourcesView.vue') },
  { path: '/resources/:id', name: 'resource-detail', component: () => import('@/views/ResourceDetailView.vue') },
  { path: '/resources/:id', name: 'resource-detail', component: () => import('@/views/ResourceDetailView.vue') },

  /* ---------- 后台管理 ---------- */
  {
    path: '/admin/login',
    name: 'admin-login',
    component: () => import('@/views/admin/LoginView.vue'),
  },
  {
    path: '/admin',
    component: () => import('@/components/admin/AdminLayout.vue'),
    children: [
      {
        path: '',
        name: 'admin-dashboard',
        component: () => import('@/views/admin/DashboardView.vue'),
      },
      {
        path: 'posts',
        name: 'admin-posts',
        component: () => import('@/views/admin/PostManageView.vue'),
      },
      {
        path: 'posts/new',
        name: 'admin-post-new',
        component: () => import('@/views/admin/PostEditView.vue'),
      },
      {
        path: 'posts/:id/edit',
        name: 'admin-post-edit',
        component: () => import('@/views/admin/PostEditView.vue'),
      },
      {
        path: 'comments',
        name: 'admin-comments',
        component: () => import('@/views/admin/CommentManageView.vue'),
      },
      {
        path: 'comments/:id/context',
        name: 'admin-comment-context',
        component: () => import('@/views/admin/CommentContextView.vue'),
      },
      {
        path: 'links',
        name: 'admin-links',
        component: () => import('@/views/admin/LinkManageView.vue'),
      },
      {
        path: 'site',
        name: 'admin-site',
        component: () => import('@/views/admin/SiteManageView.vue'),
      },
      { path: 'resources', name: 'admin-resources', component: () => import('@/views/admin/ResourceManageView.vue') },
      { path: 'music', name: 'admin-music', component: () => import('@/views/admin/MusicManageView.vue') },
      { path: 'backups', name: 'admin-backups', component: () => import('@/views/admin/BackupManageView.vue') },
      { path: 'password', name: 'admin-password', component: () => import('@/views/admin/PasswordView.vue') },
    ],
  },

  {
    path: '/:pathMatch(.*)*',
    name: 'notfound',
    component: () => import('@/views/NotFoundView.vue'),
  },
]

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes,
  scrollBehavior() {
    return { top: 0 }
  },
})

// 登录守卫
router.beforeEach((to) => {
  if (to.path.startsWith('/admin') && to.name !== 'admin-login' && !isLoggedIn()) {
    return { name: 'admin-login', query: { redirect: to.fullPath } }
  }
  if (to.name === 'admin-login' && isLoggedIn()) {
    return { path: '/admin' }
  }
  return true
})

router.afterEach((to, _from, failure) => {
  if (!failure) trackVisit(to.path)
})
window.addEventListener('blog-reader-session', () => trackVisit(window.location.pathname))

export default router
