<script setup lang="ts">
</script>

<template>
  <div class="container">
    <div class="nf">
      <p class="nf__code">$ cd /this/page</p>
      <p class="nf__err">bash: cd: no such file or directory: 404</p>
      <h1 class="nf__title">页面不存在</h1>
      <p class="nf__sub">你访问的页面可能已被移动或删除。</p>
      <router-link to="/" class="nf__link">← 回到首页</router-link>
    </div>
  </div>
</template>

<style scoped>
.nf {
  padding: 70px 0;
  text-align: center;
}
.nf__code {
  font-family: var(--font-mono);
  font-size: 14px;
  color: var(--ink-faint);
  margin-bottom: 6px;
}
.nf__err {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--amber);
  margin-bottom: 28px;
}
.nf__title { font-size: 30px; margin-bottom: 10px; }
.nf__sub {
  font-size: 15px;
  color: var(--ink-faint);
  margin-bottom: 28px;
}
.nf__link {
  display: inline-block;
  padding: 10px 22px;
  background: var(--accent);
  color: #fff;
  border-radius: var(--radius);
  font-size: 14.5px;
}
.nf__link:hover { background: var(--accent-hover); color: #fff; }
</style>
