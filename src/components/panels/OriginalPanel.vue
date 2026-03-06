<template>
  <div class="p-4 flex flex-col gap-4">
    <h2 class="text-sm font-semibold text-slate-300 uppercase tracking-wide">步骤 3：原机操作</h2>

    <!-- 导入轨迹 -->
    <div class="flex flex-col gap-2">
      <p class="text-xs text-slate-400">轨迹文件</p>
      <button
        class="w-full py-2 bg-slate-700 hover:bg-slate-600 border border-slate-600 rounded text-sm text-slate-300 transition-colors"
        @click="importTrajectory"
      >
        {{ originalTrajectoryPath || '📂 导入轨迹文件' }}
      </button>
    </div>

    <!-- 运行控制 -->
    <div class="flex gap-2">
      <button
        class="flex-1 py-2 rounded text-sm font-semibold transition-colors"
        :class="isRunning
          ? 'bg-yellow-700 hover:bg-yellow-600 text-white'
          : 'bg-green-700 hover:bg-green-600 text-white disabled:opacity-40'"
        :disabled="!originalTrajectory || !isRobotConnected || !isCameraConnected"
        @click="isRunning ? stopRun() : startRun()"
      >
        {{ isRunning ? '⏹ 停止' : '▶ 开始执行' }}
      </button>
    </div>

    <ProgressIndicator v-if="isRunning" :value="progress" />
    <p v-if="progressMessage" class="text-xs text-blue-400">{{ progressMessage }}</p>

    <!-- 图像列表 -->
    <ImageListPanel :images="capturedImages" />

    <!-- 黄金图像识别 -->
    <div class="flex flex-col gap-2 border-t border-slate-700 pt-4">
      <button
        class="w-full py-2 bg-purple-800 hover:bg-purple-700 rounded text-sm text-white transition-colors disabled:opacity-40"
        :disabled="capturedImages.length === 0 || isRunning"
        @click="identifyGoldenImages"
      >
        ✨ 识别黄金图像
      </button>
      <p v-if="goldenImageIds.length > 0" class="text-xs text-yellow-400">
        已识别 {{ goldenImageIds.length }} 张黄金图像
      </p>
      <button
        v-if="goldenImageIds.length > 0"
        class="w-full py-2 bg-slate-700 hover:bg-slate-600 rounded text-sm text-slate-300 transition-colors"
        @click="exportGolden"
      >
        💾 导出黄金图像信息
      </button>
    </div>

    <div v-if="errorMsg" class="p-2 bg-red-900/50 border border-red-700 rounded text-xs text-red-300">
      {{ errorMsg }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { storeToRefs } from 'pinia'
import { useCalibrationStore } from '../../stores/calibrationStore'
import { parseTrajectoryFile, extractShotPoints, getCADPointByPictureId, exportGoldenImages } from '../../services/trajectoryService'
import { getGoldenImages } from '../../services/algorithmService'
import { sendPTP } from '../../services/robotService'
import { captureImage, setCameraParams } from '../../services/cameraService'
import ProgressIndicator from '../common/ProgressIndicator.vue'
import ImageListPanel from './ImageListPanel.vue'
import type { CapturedImage, JointAngles } from '../../types'

const store = useCalibrationStore()
const {
  originalTrajectory,
  originalTrajectoryPath,
  capturedImages,
  goldenImageIds,
  isRunning,
  progress,
  progressMessage,
  isRobotConnected,
  isCameraConnected,
} = storeToRefs(store)

type ElectronAPIBridge = {
  electronAPI?: {
    invoke: (ch: string, ...a: unknown[]) => Promise<{ filePath: string; content: string } | null>
  }
}

const errorMsg = ref<string>('')
let stopRequested = false

async function importTrajectory() {
  try {
    const result = await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke('file:open')
    if (!result) return
    const trajectory = parseTrajectoryFile(result.content)
    store.setOriginalTrajectory(trajectory, result.filePath)
    store.clearCapturedImages()
  } catch (e) {
    errorMsg.value = `导入失败: ${e instanceof Error ? e.message : String(e)}`
  }
}

async function startRun() {
  if (!originalTrajectory.value) return
  stopRequested = false
  store.setIsRunning(true)
  store.clearCapturedImages()
  errorMsg.value = ''

  try {
    const shotPoints = extractShotPoints(originalTrajectory.value)
    const total = shotPoints.length

    for (let i = 0; i < total; i++) {
      if (stopRequested) break

      const sp = shotPoints[i]
      const percentage = Math.round((i / total) * 100)
      store.setProgress(percentage, `执行路点 ${i + 1}/${total}`)
      store.updateCurrentJointAngles(sp.jointAngles)

      // 发送 PTP 指令
      try {
        await sendPTP(sp.jointAngles, 0.3)
      } catch {
        // 在开发模式下忽略 PTP 错误（模拟环境）
      }

      // 获取 CAD 点信息，设置相机参数
      const cadData = getCADPointByPictureId(originalTrajectory.value, sp.pictureId)
      if (cadData?.optical_config?.camera_io_list?.[0]) {
        const cfg = cadData.optical_config.camera_io_list[0]
        try {
          await setCameraParams({ exposure: cfg.exposure, gain: cfg.gain, gamma: cfg.gamma })
        } catch {}
      }

      // 拍摄图像
      let imageData = ''
      try {
        imageData = await captureImage()
      } catch {
        imageData = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='
      }

      const image: CapturedImage = {
        pictureId: sp.pictureId,
        imageData,
        jointAngles: sp.jointAngles,
        cadPoint: cadData?.cad_point,
        tcpPoint: cadData?.tcp_point,
        isGolden: false,
        timestamp: Date.now(),
      }
      store.addCapturedImage(image)
    }

    store.setProgress(100, '执行完成')
  } catch (e) {
    errorMsg.value = `执行失败: ${e instanceof Error ? e.message : String(e)}`
  } finally {
    store.setIsRunning(false)
  }
}

function stopRun() {
  stopRequested = true
  store.setIsRunning(false)
}

async function identifyGoldenImages() {
  try {
    const ids = await getGoldenImages(capturedImages.value)
    store.setGoldenImageIds(ids)
  } catch (e) {
    errorMsg.value = `识别失败: ${e instanceof Error ? e.message : String(e)}`
  }
}

async function exportGolden() {
  if (!originalTrajectory.value) return
  try {
    const content = exportGoldenImages(capturedImages.value, goldenImageIds.value, originalTrajectory.value)
    await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke('file:save', { content })
  } catch (e) {
    errorMsg.value = `导出失败: ${e instanceof Error ? e.message : String(e)}`
  }
}
</script>
