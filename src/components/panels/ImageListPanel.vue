<template>
  <div class="flex flex-col gap-2">
    <div class="flex items-center justify-between">
      <p class="text-xs text-slate-400 font-semibold uppercase tracking-wide">
        已捕获图像 ({{ images.length }})
      </p>
    </div>
    <div class="flex flex-col gap-2 overflow-y-auto max-h-64">
      <div
        v-for="img in images"
        :key="img.pictureId"
        class="flex gap-2 p-2 rounded border cursor-pointer transition-all"
        :class="[
          img.isGolden ? 'border-yellow-500/60 bg-yellow-900/20' : 'border-slate-600 bg-slate-700/50',
          selectedImageId === img.pictureId ? 'ring-1 ring-blue-400' : ''
        ]"
        @click="store.selectedImageId = img.pictureId"
      >
        <img
          :src="img.imageData"
          class="w-12 h-12 rounded object-cover bg-slate-900 shrink-0"
          :alt="`图像 ${img.pictureId}`"
        />
        <div class="flex flex-col justify-center gap-1 min-w-0">
          <div class="flex items-center gap-1">
            <span class="text-xs text-slate-200 font-mono">ID: {{ img.pictureId }}</span>
            <span v-if="img.isGolden" class="text-xs text-yellow-400">⭐</span>
          </div>
          <span class="text-xs text-slate-500 truncate">
            J: {{ img.jointAngles.map(v => v.toFixed(1)).join(', ') }}
          </span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { useCalibrationStore } from '../../stores/calibrationStore'
import type { CapturedImage } from '../../types'

defineProps<{ images: CapturedImage[] }>()

const store = useCalibrationStore()
const { selectedImageId } = storeToRefs(store)
</script>
