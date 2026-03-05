import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { SceneSettings, JointAngles } from '../types'

export const useSceneStore = defineStore('scene', () => {
  // 场景显示设置
  const settings = ref<SceneSettings>({
    backgroundColor: '#1e293b',
    showGrid: true,
    showAxes: true,
    pointSize: 5,
    lineWidth: 1,
  })

  // 机器人模型状态
  const urdfModelPath = ref<string>('')
  const robotLinkFiles = ref<Record<string, string>>({})
  const currentPose = ref<JointAngles>([0, 0, 0, 0, 0, 0])

  // 场景内容
  const showTrajectoryPoints = ref<boolean>(true)
  const showCADPoints = ref<boolean>(true)
  const showActualTrajectory = ref<boolean>(true)
  const showPlannedTrajectory = ref<boolean>(true)
  const showFOVRectangles = ref<boolean>(false)

  // 操作
  function updateSettings(newSettings: Partial<SceneSettings>) {
    settings.value = { ...settings.value, ...newSettings }
  }

  function setURDFModelPath(path: string) {
    urdfModelPath.value = path
  }

  function setCurrentPose(pose: JointAngles) {
    currentPose.value = pose
  }

  function toggleGrid() {
    settings.value.showGrid = !settings.value.showGrid
  }

  function toggleAxes() {
    settings.value.showAxes = !settings.value.showAxes
  }

  return {
    settings,
    urdfModelPath,
    robotLinkFiles,
    currentPose,
    showTrajectoryPoints,
    showCADPoints,
    showActualTrajectory,
    showPlannedTrajectory,
    showFOVRectangles,
    updateSettings,
    setURDFModelPath,
    setCurrentPose,
    toggleGrid,
    toggleAxes,
  }
})
