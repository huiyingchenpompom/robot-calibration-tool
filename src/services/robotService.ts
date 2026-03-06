// 机器人臂控制服务 - 通过 Electron IPC 与主进程通信
import type { JointAngles } from '../types'
import { getElectronAPI } from './electronBridge'

/** IPC 通道名称 */
const IPC = {
  CONNECT: 'robot:connect',
  DISCONNECT: 'robot:disconnect',
  PTP: 'robot:ptp',
  GET_JOINTS: 'robot:getJoints',
  EMERGENCY_STOP: 'robot:emergencyStop',
  GET_STATUS: 'robot:getStatus',
}

/**
 * 连接机器人臂（调用协议层 Connect RPC 握手）
 */
export async function connectRobot(ip: string, port: number): Promise<void> {
  const result = await getElectronAPI().invoke(IPC.CONNECT, { ip, port }) as { success: boolean; error?: string }
  if (!result.success) {
    throw new Error(result.error ?? '机器人连接失败')
  }
}

/**
 * 断开机器人臂连接
 */
export async function disconnectRobot(): Promise<void> {
  await getElectronAPI().invoke(IPC.DISCONNECT)
}

/**
 * 发送 PTP 运动指令
 */
export async function sendPTP(
  jointAngles: JointAngles,
  speed: number = 0.3
): Promise<void> {
  await getElectronAPI().invoke(IPC.PTP, { jointAngles, speed })
}

/**
 * 获取当前关节角度
 */
export async function getCurrentJoints(): Promise<JointAngles> {
  const result = await getElectronAPI().invoke(IPC.GET_JOINTS)
  return result as JointAngles
}

/**
 * 紧急停止
 */
export async function emergencyStop(): Promise<void> {
  await getElectronAPI().invoke(IPC.EMERGENCY_STOP)
}

/**
 * 获取机器人完整状态（state / joints / is_powered）
 */
export interface RobotStatus {
  state: string
  joints: JointAngles
  is_powered: boolean
  error_message: string
}

export async function getRobotStatus(): Promise<RobotStatus> {
  const result = await getElectronAPI().invoke(IPC.GET_STATUS)
  return result as RobotStatus
}
