<template>
  <footer class="flex items-center justify-between px-4 h-8 bg-slate-950 border-t border-slate-700 text-xs text-slate-400 shrink-0">
    <div class="flex items-center gap-4">
      <span>关节: {{ joints }}</span>
      <span v-if="isRunning" class="text-blue-400 animate-pulse">⟳ {{ progressMessage }}</span>
    </div>
    <div class="flex items-center gap-4">
      <span v-if="originalTrajectory">轨迹: 已加载 ({{ waypointCount }} 路点)</span>
      <ProgressIndicator v-if="isRunning" :value="progress" class="w-32" />
      <span>{{ currentStep }}</span>
    </div>
  </footer>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { storeToRefs } from 'pinia'
import { useCalibrationStore } from '../../stores/calibrationStore'
import ProgressIndicator from '../common/ProgressIndicator.vue'

const store = useCalibrationStore()
const { currentJointAngles, isRunning, progress, progressMessage, originalTrajectory, currentStep } = storeToRefs(store)

const joints = computed(() =>
  currentJointAngles.value.map(v => v.toFixed(2)).join(', ')
)

const waypointCount = computed(() => {
  if (!originalTrajectory.value) return 0
  return Object.values(originalTrajectory.value.flying_shots).reduce(
    (sum, shot) => sum + shot.traj_waypoints.length,
    0
  )
})
</script>
