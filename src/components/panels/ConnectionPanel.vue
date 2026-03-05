<template>
  <div class="p-4 flex flex-col gap-6">
    <h2 class="text-sm font-semibold text-slate-300 uppercase tracking-wide">步骤 1：连接</h2>

    <!-- 机器人臂 -->
    <div class="flex flex-col gap-3">
      <h3 class="text-xs font-semibold text-blue-400 uppercase tracking-wide">机器人臂 (gRPC)</h3>
      <div class="flex flex-col gap-2">
        <label class="text-xs text-slate-400">IP 地址</label>
        <input
          v-model="robotConfig.ip"
          class="w-full px-2 py-1.5 bg-slate-700 border border-slate-600 rounded text-sm text-slate-200 focus:outline-none focus:border-blue-500"
          placeholder="192.168.1.100"
        />
      </div>
      <div class="flex flex-col gap-2">
        <label class="text-xs text-slate-400">端口</label>
        <input
          v-model.number="robotConfig.port"
          type="number"
          class="w-full px-2 py-1.5 bg-slate-700 border border-slate-600 rounded text-sm text-slate-200 focus:outline-none focus:border-blue-500"
          placeholder="30003"
        />
      </div>
      <button
        class="w-full py-2 rounded text-sm font-semibold transition-colors"
        :class="isRobotConnected
          ? 'bg-red-800 hover:bg-red-700 text-red-200'
          : 'bg-blue-700 hover:bg-blue-600 text-white'"
        :disabled="robotConnectionState === 'connecting'"
        @click="toggleRobotConnection"
      >
        {{ robotConnectionState === 'connecting' ? '连接中…' : isRobotConnected ? '断开机器人' : '连接机器人' }}
      </button>
    </div>

    <div class="border-t border-slate-700" />

    <!-- 相机 -->
    <div class="flex flex-col gap-3">
      <h3 class="text-xs font-semibold text-blue-400 uppercase tracking-wide">相机</h3>
      <div class="flex flex-col gap-2">
        <label class="text-xs text-slate-400">相机品牌</label>
        <select
          v-model="cameraConfig.brand"
          class="w-full px-2 py-1.5 bg-slate-700 border border-slate-600 rounded text-sm text-slate-200 focus:outline-none focus:border-blue-500"
        >
          <option value="basler">Basler</option>
          <option value="hik">海康威视 (HIK)</option>
          <option value="daheng">大恒 (Daheng)</option>
        </select>
      </div>
      <div class="flex flex-col gap-2">
        <label class="text-xs text-slate-400">设备序号</label>
        <input
          v-model.number="cameraConfig.deviceIndex"
          type="number"
          class="w-full px-2 py-1.5 bg-slate-700 border border-slate-600 rounded text-sm text-slate-200 focus:outline-none focus:border-blue-500"
          placeholder="0"
        />
      </div>
      <button
        class="w-full py-2 rounded text-sm font-semibold transition-colors"
        :class="isCameraConnected
          ? 'bg-red-800 hover:bg-red-700 text-red-200'
          : 'bg-blue-700 hover:bg-blue-600 text-white'"
        :disabled="cameraConnectionState === 'connecting'"
        @click="toggleCameraConnection"
      >
        {{ cameraConnectionState === 'connecting' ? '连接中…' : isCameraConnected ? '断开相机' : '连接相机' }}
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
import { connectRobot, disconnectRobot } from '../../services/robotService'
import { connectCamera, disconnectCamera } from '../../services/cameraService'

const store = useCalibrationStore()
const { robotConnectionState, cameraConnectionState, robotConfig, cameraConfig, isRobotConnected, isCameraConnected } = storeToRefs(store)

const errorMsg = ref<string>('')

async function toggleRobotConnection() {
  errorMsg.value = ''
  if (isRobotConnected.value) {
    store.setRobotConnectionState('disconnected')
    try { await disconnectRobot() } catch {}
  } else {
    store.setRobotConnectionState('connecting')
    try {
      await connectRobot(robotConfig.value.ip, robotConfig.value.port)
      store.setRobotConnectionState('connected')
    } catch (e) {
      store.setRobotConnectionState('error')
      errorMsg.value = `机器人连接失败: ${e instanceof Error ? e.message : String(e)}`
    }
  }
}

async function toggleCameraConnection() {
  errorMsg.value = ''
  if (isCameraConnected.value) {
    store.setCameraConnectionState('disconnected')
    try { await disconnectCamera() } catch {}
  } else {
    store.setCameraConnectionState('connecting')
    try {
      await connectCamera(cameraConfig.value.brand, cameraConfig.value.deviceIndex)
      store.setCameraConnectionState('connected')
    } catch (e) {
      store.setCameraConnectionState('error')
      errorMsg.value = `相机连接失败: ${e instanceof Error ? e.message : String(e)}`
    }
  }
}
</script>
