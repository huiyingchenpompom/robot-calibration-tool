// 相机服务 - 通过 Electron IPC 与主进程通信（主进程通过 native addon 控制相机）
import type { CameraBrand } from '../types'
import { getElectronAPI } from './electronBridge'

/** IPC 通道名称 */
const IPC = {
  CONNECT: 'camera:connect',
  DISCONNECT: 'camera:disconnect',
  CAPTURE: 'camera:capture',
  SET_PARAMS: 'camera:setParams',
}

/**
 * 连接相机
 */
export async function connectCamera(brand: CameraBrand, deviceIndex: number): Promise<void> {
  await getElectronAPI().invoke(IPC.CONNECT, { brand, deviceIndex })
}

/**
 * 断开相机连接
 */
export async function disconnectCamera(): Promise<void> {
  await getElectronAPI().invoke(IPC.DISCONNECT)
}

/**
 * 拍摄图像
 * @returns base64 编码的图像数据
 */
export async function captureImage(): Promise<string> {
  const result = await getElectronAPI().invoke(IPC.CAPTURE)
  return result as string
}

/**
 * 设置相机参数
 */
export async function setCameraParams(params: {
  exposure?: number
  gain?: number
  gamma?: number
}): Promise<void> {
  await getElectronAPI().invoke(IPC.SET_PARAMS, params)
}
