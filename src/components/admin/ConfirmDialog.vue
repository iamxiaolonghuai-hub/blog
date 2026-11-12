<script setup lang="ts">
defineProps<{
  open: boolean
  title: string
  message: string
  confirmText?: string
}>()

defineEmits<{
  confirm: []
  cancel: []
}>()
</script>

<template>
  <teleport to="body">
    <div v-if="open" class="confirm">
      <div class="confirm__mask" @click="$emit('cancel')"></div>
      <div class="confirm__box" role="dialog" aria-modal="true">
        <h3 class="confirm__title">{{ title }}</h3>
        <p class="confirm__msg">{{ message }}</p>
        <div class="confirm__actions">
          <button class="confirm__btn-ghost" @click="$emit('cancel')">取消</button>
          <button class="confirm__btn-danger" @click="$emit('confirm')">
            {{ confirmText || '确认删除' }}
          </button>
        </div>
      </div>
    </div>
  </teleport>
</template>

<style scoped>
.confirm {
  position: fixed;
  inset: 0;
  z-index: 100;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
}
.confirm__mask {
  position: absolute;
  inset: 0;
  background: rgba(20, 23, 27, 0.5);
}
.confirm__box {
  position: relative;
  width: 100%;
  max-width: 380px;
  background: var(--bg-raised);
  border: 1px solid var(--line);
  border-radius: var(--radius-lg);
  padding: 24px;
  box-shadow: 0 20px 60px rgba(0,0,0,0.25);
}
.confirm__title { font-size: 17px; margin-bottom: 10px; }
.confirm__msg {
  font-size: 14px;
  color: var(--ink-soft);
  line-height: 1.6;
  margin-bottom: 22px;
}
.confirm__actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}
.confirm__btn-ghost {
  padding: 8px 16px;
  background: none;
  border: 1px solid var(--line-strong);
  border-radius: var(--radius);
  font-size: 14px;
  color: var(--ink);
}
.confirm__btn-ghost:hover { border-color: var(--accent); }
.confirm__btn-danger {
  padding: 8px 16px;
  background: #b3351f;
  border: none;
  border-radius: var(--radius);
  font-size: 14px;
  color: #fff;
}
.confirm__btn-danger:hover { background: #992c19; }
</style>
