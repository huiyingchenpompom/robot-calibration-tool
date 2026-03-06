<template>
  <div class="p-4 flex flex-col gap-6">
    <h2 class="text-sm font-semibold text-slate-300 uppercase tracking-wide">场景模型</h2>

    <!-- 轨迹模型 -->
    <div class="flex flex-col gap-2">
      <h3 class="text-xs font-semibold text-blue-400 uppercase tracking-wide">轨迹模型</h3>
      <p class="text-xs text-slate-400">待检测轨迹的三维模型 (.stl)</p>
      <button
        class="w-full py-2 bg-slate-700 hover:bg-slate-600 border border-slate-600 rounded text-sm text-slate-300 transition-colors truncate"
        @click="importTrajectoryModel"
      >
        {{ trajectoryModelName || '📂 导入轨迹模型' }}
      </button>
      <p v-if="trajectoryModelName" class="text-xs text-green-400">✓ 已加载：{{ trajectoryModelName }}</p>
    </div>

    <div class="border-t border-slate-700" />

    <!-- 机台模型 -->
    <div class="flex flex-col gap-2">
      <h3 class="text-xs font-semibold text-blue-400 uppercase tracking-wide">机台模型</h3>
      <p class="text-xs text-slate-400">工作台 / 夹具的三维模型 (.stl)</p>
      <button
        class="w-full py-2 bg-slate-700 hover:bg-slate-600 border border-slate-600 rounded text-sm text-slate-300 transition-colors truncate"
        @click="importPlatformModel"
      >
        {{ platformModelName || '📂 导入机台模型' }}
      </button>
      <p v-if="platformModelName" class="text-xs text-green-400">✓ 已加载：{{ platformModelName }}</p>
    </div>

    <div class="border-t border-slate-700" />

    <!-- 机械臂模型 -->
    <div class="flex flex-col gap-2">
      <h3 class="text-xs font-semibold text-blue-400 uppercase tracking-wide">机械臂模型</h3>
      <p class="text-xs text-slate-400">选择包含 URDF 文件和所有 STL 文件的目录，一键导入完整机械臂模型</p>
      <button
        class="w-full py-2 bg-blue-700 hover:bg-blue-600 border border-blue-500 rounded text-sm text-white font-semibold transition-colors"
        :disabled="isLoadingRobot"
        @click="importRobotFolder"
      >
        {{ isLoadingRobot ? '加载中…' : (robotUrdfName ? '🔄 重新导入机械臂目录' : '📁 一键导入机械臂目录') }}
      </button>
      <div v-if="robotUrdfName" class="flex flex-col gap-1">
        <p class="text-xs text-green-400">✓ URDF：{{ robotUrdfName }}</p>
        <p class="text-xs text-slate-400">已加载 {{ robotStlCount }} 个 STL 文件</p>
      </div>
    </div>

    <div v-if="errorMsg || modelLoadError" class="p-2 bg-red-900/50 border border-red-700 rounded text-xs text-red-300">
      {{ errorMsg || modelLoadError }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { storeToRefs } from 'pinia'
import { useSceneStore } from '../../stores/sceneStore'

type ElectronAPIBridge = {
  electronAPI?: {
    invoke: (ch: string, ...a: unknown[]) => Promise<unknown>
  }
}

const sceneStore = useSceneStore()
const { trajectoryModelName, platformModelName, robotUrdfName, robotStlFiles, modelLoadError } = storeToRefs(sceneStore)

const isLoadingRobot = ref(false)
const errorMsg = ref('')

const robotStlCount = computed(() => robotStlFiles.value.size)

async function importTrajectoryModel() {
  errorMsg.value = ''
  sceneStore.clearModelLoadError()
  try {
    const result = await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke(
      'file:open-binary',
      [{ name: 'STL 模型文件', extensions: ['stl'] }]
    ) as { filePath: string; name: string; data: string } | null
    if (!result) return
    sceneStore.setTrajectoryModel(result.data, result.name)
  } catch (e) {
    errorMsg.value = `导入轨迹模型失败: ${e instanceof Error ? e.message : String(e)}`
  }
}

async function importPlatformModel() {
  errorMsg.value = ''
  sceneStore.clearModelLoadError()
  try {
    const result = await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke(
      'file:open-binary',
      [{ name: 'STL 模型文件', extensions: ['stl'] }]
    ) as { filePath: string; name: string; data: string } | null
    if (!result) return
    sceneStore.setPlatformModel(result.data, result.name)
  } catch (e) {
    errorMsg.value = `导入机台模型失败: ${e instanceof Error ? e.message : String(e)}`
  }
}

async function importRobotFolder() {
  errorMsg.value = ''
  sceneStore.clearModelLoadError()
  isLoadingRobot.value = true
  try {
    type RobotFolderResult = {
      error?: string
      dirPath: string
      urdfName: string
      urdfContent: string
      stlFiles: Array<{ name: string; data: string }>
    }
    const result = await (window as typeof window & ElectronAPIBridge).electronAPI?.invoke(
      'file:open-robot-folder'
    ) as RobotFolderResult | null
    if (!result) return
    if (result.error) {
      errorMsg.value = result.error
      return
    }
    const stlMap = new Map<string, string>(result.stlFiles.map(f => [f.name, f.data]))
    sceneStore.setRobotModels(result.urdfContent, result.urdfName, stlMap)
  } catch (e) {
    errorMsg.value = `导入机械臂模型失败: ${e instanceof Error ? e.message : String(e)}`
  } finally {
    isLoadingRobot.value = false
  }
}
</script>
