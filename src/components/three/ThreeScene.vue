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
const { settings, trajectoryModelData, platformModelData, robotUrdfContent, robotStlFiles } = storeToRefs(sceneStore)
const { currentJointAngles, originalTrajectory, goldenImageIds } = storeToRefs(calibStore)

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

watch(trajectoryModelData, (data) => {
  try {
    sceneManager?.setTrajectoryModel(data)
    if (data) sceneStore.clearModelLoadError()
  } catch (e) {
    sceneStore.setModelLoadError(`加载轨迹模型失败: ${e instanceof Error ? e.message : String(e)}`)
  }
})

watch(platformModelData, (data) => {
  try {
    sceneManager?.setPlatformModel(data)
    if (data) sceneStore.clearModelLoadError()
  } catch (e) {
    sceneStore.setModelLoadError(`加载机台模型失败: ${e instanceof Error ? e.message : String(e)}`)
  }
})

watch([robotUrdfContent, robotStlFiles], ([urdfContent, stlFiles]) => {
  try {
    sceneManager?.setRobotModels(urdfContent, stlFiles)
    if (urdfContent) sceneStore.clearModelLoadError()
  } catch (e) {
    sceneStore.setModelLoadError(`加载机械臂模型失败: ${e instanceof Error ? e.message : String(e)}`)
  }
}, { deep: true })

// 轨迹文件导入后：将所有拍照点以矩形块可视化到 3D 场景
watch([originalTrajectory, goldenImageIds], ([traj, gIds]) => {
  try {
    const pictureIdList = traj?.view_point_CAD?.picture_id_list ?? []
    const goldenSet = new Set(gIds as number[])
    sceneManager?.setShotPoints(pictureIdList, goldenSet)
  } catch (e) {
    console.warn('[ThreeScene] setShotPoints 失败:', e)
  }
}, { deep: true })

function resetCamera() {
  sceneManager?.resetCamera()
}
</script>
