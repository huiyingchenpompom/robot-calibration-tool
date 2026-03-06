<template>
  <header class="flex items-center justify-between px-4 h-12 bg-slate-900 border-b border-slate-700 shrink-0 z-10">
    <div class="flex items-center gap-3">
      <span class="text-blue-400 text-lg font-bold tracking-wide">🤖 Robot Calibration Tool</span>
      <span class="text-slate-500 text-xs">v1.0.0</span>
    </div>
    <div class="flex items-center gap-4">
      <!-- 机器人连接状态 -->
      <div class="flex items-center gap-2">
        <span
          class="w-2 h-2 rounded-full"
          :class="{
            'bg-green-400': robotConnectionState === 'connected',
            'bg-yellow-400 animate-pulse': robotConnectionState === 'connecting',
            'bg-red-400': robotConnectionState === 'error',
            'bg-slate-500': robotConnectionState === 'disconnected',
          }"
        />
        <span class="text-xs text-slate-400">机器人臂</span>
      </div>
      <!-- 相机连接状态 -->
      <div class="flex items-center gap-2">
        <span
          class="w-2 h-2 rounded-full"
          :class="{
            'bg-green-400': cameraConnectionState === 'connected',
            'bg-yellow-400 animate-pulse': cameraConnectionState === 'connecting',
            'bg-red-400': cameraConnectionState === 'error',
            'bg-slate-500': cameraConnectionState === 'disconnected',
          }"
        />
        <span class="text-xs text-slate-400">相机</span>
      </div>
      <!-- 紧急停止按钮 -->
      <button
        class="px-3 py-1 bg-red-700 hover:bg-red-600 text-white text-xs font-semibold rounded border border-red-500 transition-colors"
        @click="handleEmergencyStop"
      >
        ⚠ 急停
      </button>
    </div>
  </header>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { useCalibrationStore } from '../../stores/calibrationStore'
import { emergencyStop } from '../../services/robotService'

const store = useCalibrationStore()
const { robotConnectionState, cameraConnectionState } = storeToRefs(store)

async function handleEmergencyStop() {
  try {
    await emergencyStop()
  } catch (e) {
    console.error('急停失败:', e)
  }
}
</script>
