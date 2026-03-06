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

  // 轨迹模型 (STL)
  const trajectoryModelData = ref<string>('')
  const trajectoryModelName = ref<string>('')

  // 机台模型 (STL)
  const platformModelData = ref<string>('')
  const platformModelName = ref<string>('')

  // 机械臂模型 (URDF + 多个 STL)
  const robotUrdfContent = ref<string>('')
  const robotUrdfName = ref<string>('')
  const robotStlFiles = ref<Map<string, string>>(new Map())

  // 机器人模型状态（保留原有字段兼容其他逻辑）
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

  function setTrajectoryModel(data: string, name: string) {
    trajectoryModelData.value = data
    trajectoryModelName.value = name
  }

  function setPlatformModel(data: string, name: string) {
    platformModelData.value = data
    platformModelName.value = name
  }

  function setRobotModels(urdfContent: string, urdfName: string, stlFiles: Map<string, string>) {
    robotUrdfContent.value = urdfContent
    robotUrdfName.value = urdfName
    robotStlFiles.value = stlFiles
  }

  // 模型加载错误（由 ThreeScene 写入，由 SceneSetupPanel 读取）
  const modelLoadError = ref<string>('')

  function setModelLoadError(msg: string) {
    modelLoadError.value = msg
  }

  function clearModelLoadError() {
    modelLoadError.value = ''
  }

  return {
    settings,
    urdfModelPath,
    robotLinkFiles,
    currentPose,
    trajectoryModelData,
    trajectoryModelName,
    platformModelData,
    platformModelName,
    robotUrdfContent,
    robotUrdfName,
    robotStlFiles,
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
    setTrajectoryModel,
    setPlatformModel,
    setRobotModels,
    modelLoadError,
    setModelLoadError,
    clearModelLoadError,
  }
})
