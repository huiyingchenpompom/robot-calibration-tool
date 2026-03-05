import { app, BrowserWindow, ipcMain, dialog } from 'electron'
import { fileURLToPath } from 'node:url'
import path from 'node:path'
import fs from 'node:fs'
import * as grpc from '@grpc/grpc-js'
import * as protoLoader from '@grpc/proto-loader'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const VITE_DEV_SERVER_URL = process.env['VITE_DEV_SERVER_URL']

let win: BrowserWindow | null = null
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

// 加载 proto 定义
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
    robotClient = new ServiceClient(`${ip}:${port}`, grpc.credentials.createInsecure())
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
    ;(robotClient as Record<string, (req: unknown, cb: (err: Error | null, res: unknown) => void) => void>).MovePTP(
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
    ;(robotClient as Record<string, (req: unknown, cb: (err: Error | null, res: Record<string, unknown>) => void) => void>).GetJointPositions(
      {},
      (err: Error | null, response: Record<string, unknown>) => {
        if (err) resolve([0, 0, 0, 0, 0, 0])
        else {
          const j = response.joints as Record<string, number>
          resolve([j.j1, j.j2, j.j3, j.j4, j.j5, j.j6])
        }
      }
    )
  })
})

// 紧急停止
ipcMain.handle('robot:emergencyStop', async () => {
  if (!robotClient) return { success: false, error: '机器人未连接' }
  return new Promise((resolve) => {
    ;(robotClient as Record<string, (req: unknown, cb: (err: Error | null, res: unknown) => void) => void>).EmergencyStop(
      {},
      (err: Error | null, response: unknown) => {
        if (err) resolve({ success: false, error: err.message })
        else resolve(response)
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
