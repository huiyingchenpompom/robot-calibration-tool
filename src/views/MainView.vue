<template>
  <div class="flex flex-1 overflow-hidden">
    <!-- 中央 3D 视口 -->
    <div class="flex-1 relative overflow-hidden">
      <ThreeScene />
      <SceneControlPanel class="absolute bottom-4 left-4" />
    </div>
    <!-- 右侧上下文面板 -->
    <div class="w-80 flex flex-col border-l border-slate-700 bg-slate-800 overflow-y-auto">
      <ConnectionPanel v-if="currentStep === 'connect'" />
      <SceneSetupPanel v-else-if="currentStep === 'scene'" />
      <OriginalPanel v-else-if="currentStep === 'original'" />
      <NewMachinePanel v-else-if="currentStep === 'new-machine'" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { useCalibrationStore } from '../stores/calibrationStore'
import ThreeScene from '../components/three/ThreeScene.vue'
import SceneControlPanel from '../components/panels/SceneControlPanel.vue'
import ConnectionPanel from '../components/panels/ConnectionPanel.vue'
import SceneSetupPanel from '../components/panels/SceneSetupPanel.vue'
import OriginalPanel from '../components/panels/OriginalPanel.vue'
import NewMachinePanel from '../components/panels/NewMachinePanel.vue'

const store = useCalibrationStore()
const { currentStep } = storeToRefs(store)
</script>
