import { app, BrowserWindow, ipcMain, dialog } from 'electron'
import { fileURLToPath } from 'node:url'
import path from 'node:path'
import fs from 'node:fs'
import * as grpc from '@grpc/grpc-js'
import * as protoLoader from '@grpc/proto-loader'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const VITE_DEV_SERVER_URL = process.env['VITE_DEV_SERVER_URL']

let win: BrowserWindow | null = null
type GrpcRobotClient = Record<string, (req: unknown, cb: (err: Error | null, res: unknown) => void) => void>
let robotClient: grpc.Client | null = null

function createWindow() {
  win = new BrowserWindow({
    width: 1600,
    height: 900,
    backgroundColor: '#0f172a',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
    },
  })

  if (VITE_DEV_SERVER_URL) {
    win.loadURL(VITE_DEV_SERVER_URL)
    win.webContents.openDevTools()
  } else {
    win.loadFile(path.join(__dirname, '../dist/index.html'))
  }
}

app.whenReady().then(createWindow)

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})

// ========================
// gRPC 机器人控制 IPC 处理
// ========================

// 提取关节数组辅助函数
function extractJoints(joints: Record<string, number>): [number, number, number, number, number, number] {
  return [joints.j1 ?? 0, joints.j2 ?? 0, joints.j3 ?? 0, joints.j4 ?? 0, joints.j5 ?? 0, joints.j6 ?? 0]
}
function loadProto() {
  const protoPath = path.join(__dirname, '../../src/proto/robot_control_3_7_0.proto')
  const packageDef = protoLoader.loadSync(protoPath, {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true,
  })
  return grpc.loadPackageDefinition(packageDef) as Record<string, grpc.GrpcObject>
}

// 连接机器人
ipcMain.handle('robot:connect', async (_event, { ip, port }: { ip: string; port: number }) => {
  try {
    const proto = loadProto()
    const RobotControl = proto['robot_control'] as grpc.GrpcObject
    const ServiceClient = (RobotControl as Record<string, typeof grpc.Client>)['RobotControl']
    const client = new ServiceClient(`${ip}:${port}`, grpc.credentials.createInsecure())
    // 按协议定义调用应用层 Connect RPC 完成握手
    const connectResult = await new Promise<{ success: boolean; message: string }>((resolve, reject) => {
      ;(client as unknown as GrpcRobotClient).Connect(
        { ip, port },
        (err: Error | null, response: unknown) => {
          if (err) reject(err)
          else resolve(response as { success: boolean; message: string })
        }
      )
    })
    if (!connectResult.success) {
      client.close()
      return { success: false, error: connectResult.message || '机器人拒绝连接' }
    }
    robotClient = client
    return { success: true }
  } catch (e) {
    return { success: false, error: String(e) }
  }
})

// 断开机器人连接
ipcMain.handle('robot:disconnect', async () => {
  if (robotClient) {
    robotClient.close()
    robotClient = null
  }
  return { success: true }
})

// PTP 运动
ipcMain.handle('robot:ptp', async (_event, { jointAngles, speed }: { jointAngles: number[]; speed: number }) => {
  if (!robotClient) return { success: false, error: '机器人未连接' }
  return new Promise((resolve) => {
    const [j1, j2, j3, j4, j5, j6] = jointAngles
    ;(robotClient as unknown as GrpcRobotClient).MovePTP(
      { target: { j1, j2, j3, j4, j5, j6 }, speed },
      (err: Error | null, response: unknown) => {
        if (err) resolve({ success: false, error: err.message })
        else resolve(response)
      }
    )
  })
})

// 获取关节角度
ipcMain.handle('robot:getJoints', async () => {
  if (!robotClient) return [0, 0, 0, 0, 0, 0]
  return new Promise((resolve) => {
    ;(robotClient as unknown as GrpcRobotClient).GetJointPositions(
      {},
      (err: Error | null, response: unknown) => {
        if (err) resolve([0, 0, 0, 0, 0, 0])
        else {
          const j = (response as Record<string, Record<string, number>>).joints ?? {}
          resolve(extractJoints(j))
        }
      }
    )
  })
})

// 紧急停止
ipcMain.handle('robot:emergencyStop', async () => {
  if (!robotClient) return { success: false, error: '机器人未连接' }
  return new Promise((resolve) => {
    ;(robotClient as unknown as GrpcRobotClient).EmergencyStop(
      {},
      (err: Error | null, response: unknown) => {
        if (err) resolve({ success: false, error: err.message })
        else resolve(response)
      }
    )
  })
})

// 获取机器人状态
ipcMain.handle('robot:getStatus', async () => {
  if (!robotClient) return { state: 'idle', joints: [0, 0, 0, 0, 0, 0], is_powered: false, error_message: '' }
  return new Promise((resolve) => {
    ;(robotClient as unknown as GrpcRobotClient).GetRobotStatus(
      {},
      (err: Error | null, response: unknown) => {
        if (err) {
          resolve({ state: 'error', joints: [0, 0, 0, 0, 0, 0], is_powered: false, error_message: err.message })
        } else {
          const res = response as { state: string; joints: Record<string, number>; is_powered: boolean; error_message: string }
          const j = res.joints ?? {}
          resolve({
            state: res.state,
            joints: extractJoints(j),
            is_powered: res.is_powered,
            error_message: res.error_message,
          })
        }
      }
    )
  })
})

// ========================
// 相机 IPC 处理（通过 native addon）
// ========================

let cameraAddon: Record<string, (...args: unknown[]) => unknown> | null = null

function loadCameraAddon() {
  if (cameraAddon) return cameraAddon
  try {
    const addonPath = path.join(__dirname, '../../camera_addon/build/Release/camera_addon.node')
    cameraAddon = require(addonPath) as Record<string, (...args: unknown[]) => unknown>
    return cameraAddon
  } catch {
    console.warn('相机 addon 未找到，使用模拟模式')
    return null
  }
}

// 连接相机
ipcMain.handle('camera:connect', async (_event, { brand, deviceIndex }: { brand: string; deviceIndex: number }) => {
  const addon = loadCameraAddon()
  if (!addon) {
    // 模拟模式
    return { success: true, mock: true }
  }
  try {
    addon.connect(brand, deviceIndex)
    return { success: true }
  } catch (e) {
    return { success: false, error: String(e) }
  }
})

// 断开相机
ipcMain.handle('camera:disconnect', async () => {
  const addon = loadCameraAddon()
  if (!addon) return { success: true }
  try {
    addon.disconnect()
    return { success: true }
  } catch (e) {
    return { success: false, error: String(e) }
  }
})

// 拍摄图像
ipcMain.handle('camera:capture', async () => {
  const addon = loadCameraAddon()
  if (!addon) {
    // 返回模拟图像（1x1 灰色像素 PNG base64）
    return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=='
  }
  try {
    const imageData = addon.captureImage() as string
    return imageData
  } catch (e) {
    throw new Error(`拍摄失败: ${String(e)}`)
  }
})

// 设置相机参数
ipcMain.handle('camera:setParams', async (_event, params: Record<string, number>) => {
  const addon = loadCameraAddon()
  if (!addon) return { success: true }
  try {
    addon.setParams(params)
    return { success: true }
  } catch (e) {
    return { success: false, error: String(e) }
  }
})

// ========================
// 文件操作 IPC 处理
// ========================

// 打开文件
ipcMain.handle('file:open', async (_event, filters?: Electron.FileFilter[]) => {
  const result = await dialog.showOpenDialog({
    properties: ['openFile'],
    filters: filters ?? [{ name: 'JSON Files', extensions: ['json'] }],
  })
  if (result.canceled || !result.filePaths[0]) return null
  const filePath = result.filePaths[0]
  const content = fs.readFileSync(filePath, 'utf-8')
  return { filePath, content }
})

// 保存文件
ipcMain.handle('file:save', async (_event, { content, filters }: { content: string; filters?: Electron.FileFilter[] }) => {
  const result = await dialog.showSaveDialog({
    filters: filters ?? [{ name: 'JSON Files', extensions: ['json'] }],
  })
  if (result.canceled || !result.filePath) return null
  fs.writeFileSync(result.filePath, content, 'utf-8')
  return result.filePath
})
