// Electron IPC 工具 - 提供类型安全的 renderer→main 通信接口

export interface ElectronAPI {
  invoke(channel: string, ...args: unknown[]): Promise<unknown>
}

/**
 * 获取 window.electronAPI，不在 Electron 环境中运行时抛出错误
 */
export function getElectronAPI(): ElectronAPI {
  const api = (window as typeof window & { electronAPI?: ElectronAPI }).electronAPI
  if (!api) {
    throw new Error(
      'Electron API 不可用。请确保应用在 Electron 环境中运行，' +
      '且 preload 脚本已正确通过 contextBridge 暴露 electronAPI。'
    )
  }
  return api
}
