<template>
  <div class="bg-slate-900/80 backdrop-blur-sm border border-slate-700 rounded-lg p-3 flex flex-col gap-3 w-56 text-xs">
    <p class="text-slate-400 font-semibold uppercase tracking-wide">场景设置</p>
    <div class="flex items-center justify-between">
      <span class="text-slate-300">网格</span>
      <button
        class="w-10 h-5 rounded-full transition-colors relative"
        :class="settings.showGrid ? 'bg-blue-600' : 'bg-slate-600'"
        @click="sceneStore.toggleGrid()"
      >
        <span
          class="absolute top-0.5 w-4 h-4 bg-white rounded-full transition-all shadow"
          :class="settings.showGrid ? 'left-5' : 'left-0.5'"
        />
      </button>
    </div>
    <div class="flex items-center justify-between">
      <span class="text-slate-300">坐标轴</span>
      <button
        class="w-10 h-5 rounded-full transition-colors relative"
        :class="settings.showAxes ? 'bg-blue-600' : 'bg-slate-600'"
        @click="sceneStore.toggleAxes()"
      >
        <span
          class="absolute top-0.5 w-4 h-4 bg-white rounded-full transition-all shadow"
          :class="settings.showAxes ? 'left-5' : 'left-0.5'"
        />
      </button>
    </div>
    <div class="flex items-center gap-2">
      <span class="text-slate-300 w-16">背景色</span>
      <input
        type="color"
        :value="settings.backgroundColor"
        class="w-8 h-6 rounded border border-slate-600 bg-transparent cursor-pointer"
        @input="(e) => sceneStore.updateSettings({ backgroundColor: (e.target as HTMLInputElement).value })"
      />
    </div>
    <div class="flex items-center gap-2">
      <span class="text-slate-300 w-16">点大小</span>
      <input
        type="range"
        :value="settings.pointSize"
        min="1" max="20" step="1"
        class="flex-1 accent-blue-500"
        @input="(e) => sceneStore.updateSettings({ pointSize: Number((e.target as HTMLInputElement).value) })"
      />
      <span class="text-slate-400 w-4">{{ settings.pointSize }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { useSceneStore } from '../../stores/sceneStore'

const sceneStore = useSceneStore()
const { settings } = storeToRefs(sceneStore)
</script>
