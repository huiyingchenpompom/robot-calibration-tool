// 轨迹文件解析和导出服务
import type { TrajectoryFile, GoldenImageInfo, CapturedImage } from '../types'

/**
 * 解析轨迹 JSON 文件
 */
export function parseTrajectoryFile(content: string): TrajectoryFile {
  try {
    const data = JSON.parse(content) as TrajectoryFile
    if (!data.flying_shots || !data.view_point_CAD) {
      throw new Error('无效的轨迹文件格式：缺少必要字段')
    }
    return data
  } catch (e) {
    throw new Error(`解析轨迹文件失败: ${e instanceof Error ? e.message : String(e)}`)
  }
}

/**
 * 从轨迹文件中提取所有拍照点（shot_flag 为 true 的点）
 */
export function extractShotPoints(trajectory: TrajectoryFile) {
  const shotPoints: Array<{
    shotKey: string
    waypointIndex: number
    pictureId: number
    jointAngles: [number, number, number, number, number, number]
  }> = []

  for (const [shotKey, flyingShot] of Object.entries(trajectory.flying_shots)) {
    flyingShot.shot_flags.forEach((isShot, idx) => {
      if (isShot && flyingShot.picture_id[idx] !== undefined) {
        shotPoints.push({
          shotKey,
          waypointIndex: idx,
          pictureId: flyingShot.picture_id[idx],
          jointAngles: flyingShot.traj_waypoints[idx] as [number, number, number, number, number, number],
        })
      }
    })
  }

  return shotPoints
}

/**
 * 根据 picture_id 获取 CAD 点信息
 */
export function getCADPointByPictureId(trajectory: TrajectoryFile, pictureId: number) {
  return trajectory.view_point_CAD.picture_id_list.find(p => p.picture_id === pictureId) ?? null
}

/**
 * 导出黄金图像信息为 JSON
 */
export function exportGoldenImages(
  capturedImages: CapturedImage[],
  goldenIds: number[],
  trajectory: TrajectoryFile
): string {
  const goldenInfo: GoldenImageInfo[] = goldenIds.map(id => {
    const image = capturedImages.find(img => img.pictureId === id)
    const cadData = getCADPointByPictureId(trajectory, id)
    if (!image || !cadData) throw new Error(`未找到图像 ID: ${id}`)
    return {
      pictureId: id,
      cadPoint: cadData.cad_point,
      tcpPoint: cadData.tcp_point,
      imageData: image.imageData,
      jointAngles: image.jointAngles,
    }
  })
  return JSON.stringify({ golden_images: goldenInfo }, null, 2)
}

/**
 * 导出新轨迹文件
 */
export function exportNewTrajectory(trajectory: TrajectoryFile): string {
  return JSON.stringify(trajectory, null, 2)
}
