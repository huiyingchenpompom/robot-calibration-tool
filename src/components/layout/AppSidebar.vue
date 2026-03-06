<template>
  <aside class="w-16 flex flex-col items-center py-4 gap-2 bg-slate-900 border-r border-slate-700 shrink-0">
    <button
      v-for="step in steps"
      :key="step.id"
      class="flex flex-col items-center gap-1 w-14 p-2 rounded-lg transition-all text-xs"
      :class="currentStep === step.id
        ? 'bg-blue-600 text-white'
        : 'text-slate-400 hover:bg-slate-800 hover:text-slate-200'"
      @click="store.setCurrentStep(step.id as WorkflowStep)"
    >
      <span class="text-lg">{{ step.icon }}</span>
      <span class="text-center leading-tight">{{ step.label }}</span>
    </button>
  </aside>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { useCalibrationStore } from '../../stores/calibrationStore'
import type { WorkflowStep } from '../../types'

const store = useCalibrationStore()
const { currentStep } = storeToRefs(store)

const steps = [
  { id: 'connect', icon: '🔗', label: '连接' },
  { id: 'scene', icon: '🗂', label: '场景' },
  { id: 'original', icon: '📸', label: '原机' },
  { id: 'new-machine', icon: '🔄', label: '新机' },
]
</script>
