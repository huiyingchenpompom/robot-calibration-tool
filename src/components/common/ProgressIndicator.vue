<template>
  <div class="w-full bg-slate-700 rounded-full h-1.5">
    <div
      class="bg-blue-500 h-1.5 rounded-full transition-all duration-300"
      :style="{ width: `${Math.min(100, Math.max(0, value))}%` }"
    />
  </div>
</template>

<script setup lang="ts">
defineProps<{ value: number }>()
</script>
