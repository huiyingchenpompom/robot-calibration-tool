<template>
  <div class="p-4 flex flex-col gap-4">
    <h2 class="text-sm font-semibold text-slate-300 uppercase tracking-wide">步骤 4：新机操作</h2>

    <!-- 导入文件 -->
    <div class="flex flex-col gap-2">
      <p class="text-xs text-slate-400">原始轨迹文件</p>
      <button
        class="w-full py-2 bg-slate-700 hover:bg-slate-600 border border-slate-600 rounded text-sm text-slate-300 transition-colors truncate"
        @click="importOriginalTrajectory"
      >
        {{ newMachineTrajectory ? '✓ 已加载轨迹' : '📂 导入轨迹文件' }}
      </button>
      <button
        class="w-full py-2 bg-slate-700 hover:bg-slate-600 border border-slate-600 rounded text-sm text-slate-300 transition-colors truncate"
        @click="importGoldenInfo"
      >
        {{ goldenImagesInfo.length ? `✓ 已加载 ${goldenImagesInfo.length} 张黄金图像` : '📂 导入黄金图像信息' }}
      </button>
    </div>

    <!-- 自动执行 -->
    <div class="flex gap-2">
      <button
        class="flex-1 py-2 rounded text-sm font-semibold transition-colors bg-green-700 hover:bg-green-600 text-white disabled:opacity-40"
        :disabled="!newMachineTrajectory || !goldenImagesInfo.length || !isRobotConnected || isRunning"
        @click="startAutoRun"
      >
        ▶ 自动执行
      </button>
    </div>

    <ProgressIndicator v-if="isRunning" :value="progress" />
    <p v-if="progressMessage" class="text-xs text-blue-400">{{ progressMessage }}</p>

    <!-- 当前黄金图像迭代 -->
    <div v-if="currentGoldenImage" class="flex flex-col gap-3 border-t border-slate-700 pt-4">
      <p class="text-xs text-slate-400 font-semibold">
        黄金图像 {{ currentGoldenIndex + 1 }} / {{ goldenImages.length }}
      </p>
      <ImageCompare
        :original-id="currentGoldenImage.pictureId"
        :original-src="currentGoldenImage.imageData"
        :new-src="newMachineCapturedImages.find(i => i.pictureId === currentGoldenImage?.pictureId)?.imageData"
      />
      <button
        class="w-full py-2 bg-blue-700 hover:bg-blue-600 rounded text-sm text-white transition-colors disabled:opacity-40"
        :disabled="isRunning || !isRobotConnected || !isCameraConnected"
        @click="captureCurrentGolden"
      >
        📷 单点迭代拍摄
      </button>
      <button
        v-if="newMachineCapturedImages.some(i => i.pictureId === currentGoldenImage?.pictureId)"
        class="w-full py-2 bg-slate-600 hover:bg-slate-500 rounded text-sm text-slate-300 transition-colors"
        @click="store.advanceGoldenIndex()"
      >
        下一张 →
      </button>
    </div>

    <!-- 计算和导出 -->
    <div v-if="newMachineCapturedImages.length === goldenImages.length && goldenImages.length > 0" class="flex flex-col gap-2 border-t border-slate-700 pt-4">
      <button
        class="w-full py-2 bg-purple-800 hover:bg-purple-700 rounded text-sm text-white transition-colors"
        @click="computeAndExport"
      >
        🔢 计算变换矩阵并导出新轨迹
      </button>
      <p v-if="transformMatrix" class="text-xs text-green-400">✓ 变换矩阵已计算</p>
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
import { parseTrajectoryFile, extractShotPoints, getCADPointByPictureId, exportNewTrajectory } from '../../services/trajectoryService'
import { computeTransformMatrix, generateNewTrajectory } from '../../services/algorithmService'
import { sendPTP } from '../../services/robotService'
import { captureImage } from '../../services/cameraService'
import ProgressIndicator from '../common/ProgressIndicator.vue'
import ImageCompare from '../common/ImageCompare.vue'
import type { CapturedImage } from '../../types'

type ElectronAPIBridge = {
  electronAPI?: {
    invoke: (ch: string, ...a: unknown[]) => Promise<{ filePath: string; content: string } | null>
  }
}

const store = useCalibrationStore()
const {
  newMachineTrajectory,
  goldenImagesInfo,
  goldenImages,
  currentGoldenImage,
  currentGoldenIndex,
  newMachineCapturedImages,
  isRunning,
  progress,
  progressMessage,
  isRobotConnected,
  isCameraConnected,
  transformMatrix,
} = storeToRefs(store)

const errorMsg = ref<string>('')
let stopRequested = false

async function importOriginalTrajectory() {
  try {
    const result = await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke('file:open')
    if (!result) return
    store.setNewMachineTrajectory(parseTrajectoryFile(result.content))
    store.clearNewMachineCapturedImages()
    store.resetGoldenIndex()
  } catch (e) {
    errorMsg.value = `导入失败: ${e instanceof Error ? e.message : String(e)}`
  }
}

async function importGoldenInfo() {
  try {
    const result = await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke('file:open')
    if (!result) return
    const parsed = JSON.parse(result.content)
    store.setGoldenImagesInfo(parsed.golden_images ?? [])
    store.setGoldenImageIds((parsed.golden_images ?? []).map((g: { pictureId: number }) => g.pictureId))
  } catch (e) {
    errorMsg.value = `导入黄金图像信息失败: ${e instanceof Error ? e.message : String(e)}`
  }
}

async function startAutoRun() {
  if (!newMachineTrajectory.value) return
  stopRequested = false
  store.setIsRunning(true)
  errorMsg.value = ''
  const goldenIds = new Set(goldenImages.value.map(g => g.pictureId))

  try {
    const shotPoints = extractShotPoints(newMachineTrajectory.value)
    const total = shotPoints.length

    for (let i = 0; i < total; i++) {
      if (stopRequested) break
      const sp = shotPoints[i]
      store.setProgress(Math.round((i / total) * 100), `路点 ${i + 1}/${total}`)
      store.updateCurrentJointAngles(sp.jointAngles)

      if (goldenIds.has(sp.pictureId)) {
        // 在黄金图像位置暂停，等待用户操作
        store.setProgress(Math.round((i / total) * 100), `等待黄金点 ID:${sp.pictureId} 操作`)
        store.setIsRunning(false)
        return
      }

      try { await sendPTP(sp.jointAngles, 0.3) } catch {}
    }

    store.setProgress(100, '自动执行完成')
  } catch (e) {
    errorMsg.value = `执行失败: ${e instanceof Error ? e.message : String(e)}`
  } finally {
    store.setIsRunning(false)
  }
}

async function captureCurrentGolden() {
  if (!currentGoldenImage.value) return
  const goldenImg = currentGoldenImage.value
  store.setIsRunning(true)
  try {
    await sendPTP(goldenImg.jointAngles, 0.1)
    const cadData = newMachineTrajectory.value
      ? getCADPointByPictureId(newMachineTrajectory.value, goldenImg.pictureId)
      : null
    const imageData = await captureImage()
    const captured: CapturedImage = {
      pictureId: goldenImg.pictureId,
      imageData,
      jointAngles: goldenImg.jointAngles,
      cadPoint: cadData?.cad_point,
      tcpPoint: cadData?.tcp_point,
      isGolden: true,
      timestamp: Date.now(),
    }
    store.addNewMachineCapturedImage(captured)
  } catch (e) {
    errorMsg.value = `拍摄失败: ${e instanceof Error ? e.message : String(e)}`
  } finally {
    store.setIsRunning(false)
  }
}

async function computeAndExport() {
  if (!newMachineTrajectory.value) return
  store.setIsRunning(true)
  try {
    const matrix = await computeTransformMatrix(goldenImagesInfo.value, newMachineCapturedImages.value)
    store.setTransformMatrix(matrix)
    const newTraj = await generateNewTrajectory(newMachineTrajectory.value, matrix)
    const content = exportNewTrajectory(newTraj)
    await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke('file:save', { content })
  } catch (e) {
    errorMsg.value = `计算/导出失败: ${e instanceof Error ? e.message : String(e)}`
  } finally {
    store.setIsRunning(false)
  }
}
</script>
