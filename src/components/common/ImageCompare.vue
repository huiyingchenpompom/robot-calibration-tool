<template>
  <div class="flex flex-col gap-2">
    <p class="text-xs text-slate-400 font-semibold uppercase tracking-wide">图像对比</p>
    <div class="flex gap-2">
      <div class="flex-1 flex flex-col gap-1">
        <p class="text-xs text-slate-500 text-center">原始图像 (ID: {{ originalId }})</p>
        <img
          v-if="originalSrc"
          :src="originalSrc"
          class="w-full rounded border border-slate-600 object-contain bg-slate-900"
          style="max-height: 160px"
        />
        <div v-else class="w-full h-28 rounded border border-slate-600 bg-slate-900 flex items-center justify-center text-slate-500 text-xs">
          无图像
        </div>
      </div>
      <div class="flex-1 flex flex-col gap-1">
        <p class="text-xs text-slate-500 text-center">新机图像</p>
        <img
          v-if="newSrc"
          :src="newSrc"
          class="w-full rounded border border-slate-600 object-contain bg-slate-900"
          style="max-height: 160px"
        />
        <div v-else class="w-full h-28 rounded border border-slate-600 bg-slate-900 flex items-center justify-center text-slate-500 text-xs">
          待拍摄
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  originalId?: number
  originalSrc?: string
  newSrc?: string
}>()
</script>
