import { defineStore } from 'pinia'
import { ref, computed, watch } from 'vue'
import type {
  TrajectoryFile,
  CapturedImage,
  RobotConnectionState,
  CameraConnectionState,
  WorkflowStep,
  RobotConfig,
  CameraConfig,
  GoldenImageInfo,
  TransformMatrix,
  JointAngles,
} from '../types'
import { getRobotStatus } from '../services/robotService'

export const useCalibrationStore = defineStore('calibration', () => {
  // 工作流状态
  const currentStep = ref<WorkflowStep>('connect')

  // 机器人状态
  const robotConnectionState = ref<RobotConnectionState>('disconnected')
  const robotConfig = ref<RobotConfig>({ ip: '192.168.1.100', port: 30003 })
  const currentJointAngles = ref<JointAngles>([0, 0, 0, 0, 0, 0])

  // 相机状态
  const cameraConnectionState = ref<CameraConnectionState>('disconnected')
  const cameraConfig = ref<CameraConfig>({ brand: 'basler', deviceIndex: 0 })

  // 轨迹数据
  const originalTrajectory = ref<TrajectoryFile | null>(null)
  const originalTrajectoryPath = ref<string>('')

  // 捕获的图像
  const capturedImages = ref<CapturedImage[]>([])
  const selectedImageId = ref<number | null>(null)

  // 黄金图像
  const goldenImageIds = ref<number[]>([])
  const goldenImagesInfo = ref<GoldenImageInfo[]>([])

  // 新机器操作
  const newMachineTrajectory = ref<TrajectoryFile | null>(null)
  const newMachineCapturedImages = ref<CapturedImage[]>([])
  const currentGoldenIndex = ref<number>(0)
  const transformMatrix = ref<TransformMatrix | null>(null)

  // 进度
  const isRunning = ref<boolean>(false)
  const progress = ref<number>(0)
  const progressMessage = ref<string>('')

  // 计算属性
  const isRobotConnected = computed(() => robotConnectionState.value === 'connected')
  const isCameraConnected = computed(() => cameraConnectionState.value === 'connected')
  const goldenImages = computed(() =>
    capturedImages.value.filter(img => goldenImageIds.value.includes(img.pictureId))
  )
  const currentGoldenImage = computed(() =>
    goldenImages.value[currentGoldenIndex.value] ?? null
  )

  // 操作
  function setCurrentStep(step: WorkflowStep) {
    currentStep.value = step
  }

  function setRobotConnectionState(state: RobotConnectionState) {
    robotConnectionState.value = state
  }

  function setCameraConnectionState(state: CameraConnectionState) {
    cameraConnectionState.value = state
  }

  function setOriginalTrajectory(trajectory: TrajectoryFile, path: string) {
    originalTrajectory.value = trajectory
    originalTrajectoryPath.value = path
  }

  function addCapturedImage(image: CapturedImage) {
    capturedImages.value.push(image)
  }

  function clearCapturedImages() {
    capturedImages.value = []
  }

  function setGoldenImageIds(ids: number[]) {
    goldenImageIds.value = ids
    // 替换数组以确保 Vue 3 响应式正确追踪黄金标记的变更
    capturedImages.value = capturedImages.value.map(img => ({
      ...img,
      isGolden: ids.includes(img.pictureId),
    }))
  }

  function setGoldenImagesInfo(info: GoldenImageInfo[]) {
    goldenImagesInfo.value = info
  }

  function setNewMachineTrajectory(trajectory: TrajectoryFile) {
    newMachineTrajectory.value = trajectory
  }

  function addNewMachineCapturedImage(image: CapturedImage) {
    newMachineCapturedImages.value.push(image)
  }

  function clearNewMachineCapturedImages() {
    newMachineCapturedImages.value = []
  }

  function setTransformMatrix(matrix: TransformMatrix) {
    transformMatrix.value = matrix
  }

  function setProgress(value: number, message: string = '') {
    progress.value = value
    progressMessage.value = message
  }

  function setIsRunning(running: boolean) {
    isRunning.value = running
  }

  function updateCurrentJointAngles(angles: JointAngles) {
    currentJointAngles.value = angles
  }

  function advanceGoldenIndex() {
    if (currentGoldenIndex.value < goldenImages.value.length - 1) {
      currentGoldenIndex.value++
    }
  }

  function resetGoldenIndex() {
    currentGoldenIndex.value = 0
  }

  // 机器人状态轮询：连接时每 500 ms 更新关节角度
  let _pollTimer: ReturnType<typeof setInterval> | null = null

  function _startJointPolling() {
    if (_pollTimer) return
    _pollTimer = setInterval(async () => {
      try {
        const status = await getRobotStatus()
        currentJointAngles.value = status.joints
      } catch (e) {
        // 开发模式下输出警告，方便排查连接问题
        if (import.meta.env.DEV) {
          console.warn('[robot poll] GetRobotStatus 失败:', e)
        }
      }
    }, 500)
  }

  function _stopJointPolling() {
    if (_pollTimer) {
      clearInterval(_pollTimer)
      _pollTimer = null
    }
  }

  const _unwatch = watch(robotConnectionState, (state) => {
    if (state === 'connected') {
      _startJointPolling()
    } else {
      _stopJointPolling()
    }
  })

  /** 销毁 store 时调用（清理轮询定时器和 watcher） */
  function $cleanup() {
    _stopJointPolling()
    _unwatch()
  }

  return {
    currentStep,
    robotConnectionState,
    robotConfig,
    currentJointAngles,
    cameraConnectionState,
    cameraConfig,
    originalTrajectory,
    originalTrajectoryPath,
    capturedImages,
    selectedImageId,
    goldenImageIds,
    goldenImagesInfo,
    newMachineTrajectory,
    newMachineCapturedImages,
    currentGoldenIndex,
    transformMatrix,
    isRunning,
    progress,
    progressMessage,
    isRobotConnected,
    isCameraConnected,
    goldenImages,
    currentGoldenImage,
    setCurrentStep,
    setRobotConnectionState,
    setCameraConnectionState,
    setOriginalTrajectory,
    addCapturedImage,
    clearCapturedImages,
    setGoldenImageIds,
    setGoldenImagesInfo,
    setNewMachineTrajectory,
    addNewMachineCapturedImage,
    clearNewMachineCapturedImages,
    setTransformMatrix,
    setProgress,
    setIsRunning,
    updateCurrentJointAngles,
    advanceGoldenIndex,
    resetGoldenIndex,
    $cleanup,
  }
})
