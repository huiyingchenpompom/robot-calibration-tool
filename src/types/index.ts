// 机器人标定工具类型定义

/** 关节角度 [j1, j2, j3, j4, j5, j6] */
export type JointAngles = [number, number, number, number, number, number]

/** 6D位姿 [x, y, z, rx, ry, rz] */
export type Pose6D = [number, number, number, number, number, number]

/** 相机配置 */
export interface CameraIOConfig {
  exposure: number
  gain: number
  gamma: number
  camera_fov_height: number
  camera_fov_width: number
}

/** 光源配置 */
export interface LightSourceIO {
  [key: string]: number | string
}

/** 光学配置 */
export interface OpticalConfig {
  camera_io_list: CameraIOConfig[]
  light_source_io_list: LightSourceIO[]
}

/** 视点CAD数据 */
export interface ViewPointCAD {
  picture_id: number
  cad_point: Pose6D
  tcp_point: Pose6D
  optical_config: OpticalConfig
}

/** 飞拍段数据 */
export interface FlyingShot {
  traj_waypoints: JointAngles[]
  shot_flags: boolean[]
  picture_id: number[]
  parent_web_point_id_list: number[]
  speed_coefficients: number[]
  standing_times: number[]
  stage_list: string[]
}

/** 轨迹文件格式 */
export interface TrajectoryFile {
  flying_shots: Record<string, FlyingShot>
  view_point_CAD: {
    picture_id_list: ViewPointCAD[]
  }
}

/** 捕获的图像信息 */
export interface CapturedImage {
  pictureId: number
  imageData: string  // base64 data URL
  jointAngles: JointAngles
  cadPoint?: Pose6D
  tcpPoint?: Pose6D
  isGolden: boolean
  timestamp: number
}

/** 机器人连接状态 */
export type RobotConnectionState = 'disconnected' | 'connecting' | 'connected' | 'error'

/** 相机连接状态 */
export type CameraConnectionState = 'disconnected' | 'connecting' | 'connected' | 'error'

/** 相机品牌 */
export type CameraBrand = 'basler' | 'hik' | 'daheng'

/** 工作流步骤 */
export type WorkflowStep = 'connect' | 'original' | 'new-machine'

/** 机器人连接配置 */
export interface RobotConfig {
  ip: string
  port: number
}

/** 相机连接配置 */
export interface CameraConfig {
  brand: CameraBrand
  deviceIndex: number
}

/** 黄金图像信息导出格式 */
export interface GoldenImageInfo {
  pictureId: number
  cadPoint: Pose6D
  tcpPoint: Pose6D
  imageData: string
  jointAngles: JointAngles
}

/** 变换矩阵 4x4 */
export type TransformMatrix = number[][]

/** 3D场景设置 */
export interface SceneSettings {
  backgroundColor: string
  showGrid: boolean
  showAxes: boolean
  pointSize: number
  lineWidth: number
}
