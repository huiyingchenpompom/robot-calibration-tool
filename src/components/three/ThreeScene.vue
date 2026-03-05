<template>
  <div ref="containerRef" class="w-full h-full relative">
    <!-- 场景覆盖信息 -->
    <div class="absolute top-2 right-2 text-xs text-slate-400 bg-slate-900/60 px-2 py-1 rounded">
      <button @click="resetCamera" class="hover:text-slate-200 transition-colors">↺ 重置视角</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, watch } from 'vue'
import { storeToRefs } from 'pinia'
import { SceneManager } from '../../three/SceneManager'
import { useSceneStore } from '../../stores/sceneStore'
import { useCalibrationStore } from '../../stores/calibrationStore'

const containerRef = ref<HTMLElement | null>(null)
let sceneManager: SceneManager | null = null

const sceneStore = useSceneStore()
const calibStore = useCalibrationStore()
const { settings } = storeToRefs(sceneStore)
const { currentJointAngles } = storeToRefs(calibStore)

onMounted(() => {
  if (containerRef.value) {
    sceneManager = new SceneManager(containerRef.value)
    sceneManager.applySettings(settings.value)
  }
})

onBeforeUnmount(() => {
  sceneManager?.dispose()
})

watch(settings, (newSettings) => {
  sceneManager?.applySettings(newSettings)
}, { deep: true })

watch(currentJointAngles, () => {
  // 更新机器人姿态（需要正向运动学，这里为占位）
})

function resetCamera() {
  sceneManager?.resetCamera()
}
</script>
