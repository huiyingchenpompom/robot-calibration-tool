// Three.js 场景管理器
import * as THREE from 'three'
import { OrbitControls, STLLoader } from 'three-stdlib'
import { parseURDF } from './URDFParser'
import type { SceneSettings, JointAngles, Pose6D, ViewPointCAD } from '../types'

// Multiplier applied to the minimum bounding-sphere-based camera distance when auto-fitting
const CAMERA_DISTANCE_MULTIPLIER = 1.8
// Default isometric-ish viewing direction (normalized inside fitCameraToObject)
const DEFAULT_CAMERA_DIRECTION = new THREE.Vector3(1, 0.7, 1)
// near/far plane scale relative to the computed camera distance
const NEAR_PLANE_FACTOR = 0.001
const FAR_PLANE_FACTOR = 100

export class SceneManager {
  private scene: THREE.Scene
  private camera: THREE.PerspectiveCamera
  private renderer: THREE.WebGLRenderer
  private controls: OrbitControls
  private animationId: number | null = null

  private gridHelper: THREE.GridHelper | null = null
  private axesHelper: THREE.AxesHelper | null = null
  private trajectoryPoints: THREE.Points | null = null
  private cadPointsGroup: THREE.Group = new THREE.Group()
  private shotPointsGroup: THREE.Group = new THREE.Group()
  private trajectoryLinesGroup: THREE.Group = new THREE.Group()

  // 导入的模型组
  private trajectoryModelMesh: THREE.Mesh | null = null
  private platformModelMesh: THREE.Mesh | null = null
  private robotModelGroup: THREE.Group = new THREE.Group()

  constructor(container: HTMLElement) {
    // 场景
    this.scene = new THREE.Scene()
    this.scene.background = new THREE.Color('#1e293b')

    // 相机
    this.camera = new THREE.PerspectiveCamera(45, container.clientWidth / container.clientHeight, 0.01, 1000)
    this.camera.position.set(2, 2, 2)
    this.camera.lookAt(0, 0, 0)

    // 渲染器
    this.renderer = new THREE.WebGLRenderer({ antialias: true })
    this.renderer.setSize(container.clientWidth, container.clientHeight)
    this.renderer.setPixelRatio(window.devicePixelRatio)
    this.renderer.shadowMap.enabled = true
    container.appendChild(this.renderer.domElement)

    // 轨道控制
    this.controls = new OrbitControls(this.camera, this.renderer.domElement)
    this.controls.enableDamping = true
    this.controls.dampingFactor = 0.05

    // 光照
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.6)
    this.scene.add(ambientLight)
    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.8)
    directionalLight.position.set(5, 10, 5)
    directionalLight.castShadow = true
    this.scene.add(directionalLight)

    // 分组
    this.scene.add(this.cadPointsGroup)
    this.scene.add(this.shotPointsGroup)
    this.scene.add(this.trajectoryLinesGroup)
    this.scene.add(this.robotModelGroup)

    // 默认辅助对象
    this.addGrid()
    this.addAxes()

    this.startAnimation()

    // 响应窗口大小变化
    const resizeObserver = new ResizeObserver(() => this.handleResize(container))
    resizeObserver.observe(container)
  }

  private startAnimation() {
    const animate = () => {
      this.animationId = requestAnimationFrame(animate)
      this.controls.update()
      this.renderer.render(this.scene, this.camera)
    }
    animate()
  }

  private handleResize(container: HTMLElement) {
    const w = container.clientWidth
    const h = container.clientHeight
    this.camera.aspect = w / h
    this.camera.updateProjectionMatrix()
    this.renderer.setSize(w, h)
  }

  /** 应用场景设置 */
  applySettings(settings: SceneSettings) {
    this.scene.background = new THREE.Color(settings.backgroundColor)
    if (this.gridHelper) this.gridHelper.visible = settings.showGrid
    if (this.axesHelper) this.axesHelper.visible = settings.showAxes
  }

  /** 添加网格 */
  private addGrid() {
    this.gridHelper = new THREE.GridHelper(10, 20, 0x334155, 0x334155)
    this.scene.add(this.gridHelper)
  }

  /** 添加坐标轴 */
  private addAxes() {
    this.axesHelper = new THREE.AxesHelper(1)
    this.scene.add(this.axesHelper)
  }

  /** 重置相机视角 */
  resetCamera() {
    this.camera.position.set(2, 2, 2)
    this.camera.lookAt(0, 0, 0)
    this.controls.reset()
  }

  /** 显示轨迹点 */
  showTrajectoryPoints(points: JointAngles[], color: number = 0x3b82f6, pointSize: number = 5) {
    if (this.trajectoryPoints) {
      this.scene.remove(this.trajectoryPoints)
    }
    if (points.length === 0) return

    const positions = new Float32Array(points.length * 3)
    points.forEach((_, idx) => {
      // 简化：直接用关节角度的前三个值作为位置（实际应做正向运动学）
      positions[idx * 3] = Math.sin(_ [0]) * 0.5
      positions[idx * 3 + 1] = _[1] * 0.1
      positions[idx * 3 + 2] = Math.cos(_[0]) * 0.5
    })

    const geometry = new THREE.BufferGeometry()
    geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3))
    const material = new THREE.PointsMaterial({ color, size: pointSize * 0.01 })
    this.trajectoryPoints = new THREE.Points(geometry, material)
    this.scene.add(this.trajectoryPoints)
  }

  /** 显示 CAD 点 */
  showCADPoints(cadPoints: Pose6D[], color: number = 0x22c55e) {
    this.cadPointsGroup.clear()
    cadPoints.forEach(pt => {
      const sphere = new THREE.Mesh(
        new THREE.SphereGeometry(0.02, 8, 8),
        new THREE.MeshStandardMaterial({ color })
      )
      sphere.position.set(pt[0], pt[1], pt[2])
      this.cadPointsGroup.add(sphere)
    })
  }

  /**
   * 显示轨迹文件的拍照点（每个 picture_id 对应一个矩形块 + 球心标记）。
   * 普通拍照点：青色；黄金图像点：金黄色。
   */
  setShotPoints(pictureIdList: ViewPointCAD[], goldenIds: Set<number> = new Set()): void {
    this.clearShotPoints()

    if (pictureIdList.length === 0) return

    pictureIdList.forEach(vp => {
      if (!vp.cad_point) return

      const isGolden = goldenIds.has(vp.picture_id)
      const color = isGolden ? 0xfbbf24 : 0x22d3ee  // 金黄 : 青色

      const [x, y, z, rx, ry, rz] = vp.cad_point
      const markerGroup = new THREE.Group()
      markerGroup.position.set(x, y, z)
      markerGroup.rotation.set(rx, ry, rz)

      // 矩形块（代表相机视野在工件上的投影区域）
      const planeGeo = new THREE.PlaneGeometry(0.04, 0.04)
      const planeMat = new THREE.MeshBasicMaterial({ color, side: THREE.DoubleSide, transparent: true, opacity: 0.75 })
      markerGroup.add(new THREE.Mesh(planeGeo, planeMat))

      // 边框线（让矩形轮廓更清晰）
      const edgesGeo = new THREE.EdgesGeometry(planeGeo)
      const edgesMat = new THREE.LineBasicMaterial({ color, transparent: true, opacity: 0.9 })
      markerGroup.add(new THREE.LineSegments(edgesGeo, edgesMat))

      // 中心球标记
      const sphereGeo = new THREE.SphereGeometry(0.008, 6, 6)
      const sphereMat = new THREE.MeshBasicMaterial({ color })
      markerGroup.add(new THREE.Mesh(sphereGeo, sphereMat))

      this.shotPointsGroup.add(markerGroup)
    })

    // 自动调整视角以完整显示所有拍照点
    if (this.shotPointsGroup.children.length > 0) {
      this.fitCameraToObject(this.shotPointsGroup)
    }
  }

  /** 释放并清空 shotPointsGroup 的所有子对象 */
  private clearShotPoints(): void {
    this.shotPointsGroup.traverse(obj => {
      if ('geometry' in obj && obj.geometry instanceof THREE.BufferGeometry) {
        obj.geometry.dispose()
      }
      if ('material' in obj && obj.material instanceof THREE.Material) {
        obj.material.dispose()
      }
    })
    this.shotPointsGroup.clear()
  }

  /** 显示轨迹线 */
  showTrajectoryLine(points: THREE.Vector3[], color: number = 0xf59e0b) {
    this.trajectoryLinesGroup.clear()
    if (points.length < 2) return
    const geometry = new THREE.BufferGeometry().setFromPoints(points)
    const material = new THREE.LineBasicMaterial({ color })
    this.trajectoryLinesGroup.add(new THREE.Line(geometry, material))
  }

  /** 清除轨迹内容 */
  clearTrajectory() {
    if (this.trajectoryPoints) {
      this.scene.remove(this.trajectoryPoints)
      this.trajectoryPoints = null
    }
    this.cadPointsGroup.clear()
    this.clearShotPoints()
    this.trajectoryLinesGroup.clear()
  }

  /** 从 base64 字符串解析 STL 几何体 */
  private loadSTLGeometry(base64Data: string): THREE.BufferGeometry {
    const binaryStr = atob(base64Data)
    const bytes = new Uint8Array(binaryStr.length)
    for (let i = 0; i < binaryStr.length; i++) {
      bytes[i] = binaryStr.charCodeAt(i)
    }
    const loader = new STLLoader()
    return loader.parse(bytes.buffer)
  }

  /** 加载并显示轨迹模型 (STL) */
  setTrajectoryModel(base64Data: string): void {
    if (this.trajectoryModelMesh) {
      this.scene.remove(this.trajectoryModelMesh)
      this.trajectoryModelMesh.geometry.dispose()
      ;(this.trajectoryModelMesh.material as THREE.Material).dispose()
      this.trajectoryModelMesh = null
    }
    if (!base64Data) return
    const geometry = this.loadSTLGeometry(base64Data)
    geometry.computeVertexNormals()
    const material = new THREE.MeshStandardMaterial({ color: 0x3b82f6, metalness: 0.2, roughness: 0.6, transparent: true, opacity: 0.85 })
    this.trajectoryModelMesh = new THREE.Mesh(geometry, material)
    this.trajectoryModelMesh.castShadow = true
    this.trajectoryModelMesh.receiveShadow = true
    this.scene.add(this.trajectoryModelMesh)
    this.fitCameraToObject(this.trajectoryModelMesh)
  }

  /** 加载并显示机台模型 (STL) */
  setPlatformModel(base64Data: string): void {
    if (this.platformModelMesh) {
      this.scene.remove(this.platformModelMesh)
      this.platformModelMesh.geometry.dispose()
      ;(this.platformModelMesh.material as THREE.Material).dispose()
      this.platformModelMesh = null
    }
    if (!base64Data) return
    const geometry = this.loadSTLGeometry(base64Data)
    geometry.computeVertexNormals()
    const material = new THREE.MeshStandardMaterial({ color: 0x64748b, metalness: 0.4, roughness: 0.5, transparent: true, opacity: 0.9 })
    this.platformModelMesh = new THREE.Mesh(geometry, material)
    this.platformModelMesh.castShadow = true
    this.platformModelMesh.receiveShadow = true
    this.scene.add(this.platformModelMesh)
    this.fitCameraToObject(this.platformModelMesh)
  }

  /** 加载并显示机械臂模型 (URDF + STL 文件集合，以零位姿显示) */
  setRobotModels(urdfContent: string, stlFiles: Map<string, string>): void {
    // 释放旧资源
    this.robotModelGroup.traverse(obj => {
      if (obj instanceof THREE.Mesh) {
        obj.geometry.dispose()
        ;(obj.material as THREE.Material).dispose()
      }
    })
    this.robotModelGroup.clear()
    if (!urdfContent || stlFiles.size === 0) return

    const robot = parseURDF(urdfContent)

    // 构建 parent link → joints 的映射
    const linkJoints = new Map<string, Array<{ child: string; originMatrix: THREE.Matrix4 }>>()
    robot.joints.forEach(joint => {
      const m = new THREE.Matrix4()
      m.makeRotationFromEuler(joint.origin.rpy)
      m.setPosition(joint.origin.xyz)
      const arr = linkJoints.get(joint.parent) ?? []
      arr.push({ child: joint.child, originMatrix: m })
      linkJoints.set(joint.parent, arr)
    })

    // 找根节点（不是任何 joint 的 child）
    const childLinkNames = new Set<string>()
    robot.joints.forEach(j => childLinkNames.add(j.child))
    let rootLinkName = ''
    robot.links.forEach((_, name) => {
      if (!childLinkNames.has(name)) rootLinkName = name
    })
    if (!rootLinkName) return

    // 深度优先遍历，累积变换矩阵后生成各 link 的 Mesh
    const traverse = (linkName: string, parentMatrix: THREE.Matrix4) => {
      const link = robot.links.get(linkName)
      if (!link) return

      if (link.visual?.geometry.type === 'mesh' && link.visual.geometry.filename) {
        const meshFileName = link.visual.geometry.filename.split('/').pop()?.toLowerCase() ?? ''
        for (const [stlName, stlData] of stlFiles) {
          if (stlName.toLowerCase() === meshFileName) {
            const geometry = this.loadSTLGeometry(stlData)
            geometry.computeVertexNormals()
            const color = link.visual.material?.color ?? new THREE.Color(0.75, 0.75, 0.8)
            const material = new THREE.MeshStandardMaterial({ color, metalness: 0.35, roughness: 0.55 })
            const mesh = new THREE.Mesh(geometry, material)

            // 视觉原点相对 link 坐标系的变换（与父变换合并后分解到 position/quaternion/scale）
            const visualMatrix = new THREE.Matrix4()
            visualMatrix.makeRotationFromEuler(link.visual.origin.rpy)
            visualMatrix.setPosition(link.visual.origin.xyz)

            const worldMatrix = parentMatrix.clone().multiply(visualMatrix)
            const pos = new THREE.Vector3()
            const quat = new THREE.Quaternion()
            const scale = new THREE.Vector3()
            worldMatrix.decompose(pos, quat, scale)
            mesh.position.copy(pos)
            mesh.quaternion.copy(quat)
            mesh.scale.copy(scale)
            mesh.castShadow = true
            this.robotModelGroup.add(mesh)
            break
          }
        }
      }

      // 递归子节点
      const children = linkJoints.get(linkName) ?? []
      for (const { child, originMatrix } of children) {
        traverse(child, parentMatrix.clone().multiply(originMatrix))
      }
    }

    traverse(rootLinkName, new THREE.Matrix4())
    if (this.robotModelGroup.children.length > 0) {
      this.fitCameraToObject(this.robotModelGroup)
    }
  }

  /** 调整相机以完整显示指定物体 */
  fitCameraToObject(object: THREE.Object3D): void {
    const box = new THREE.Box3().setFromObject(object)
    if (box.isEmpty()) return

    const center = new THREE.Vector3()
    const size = new THREE.Vector3()
    box.getCenter(center)
    box.getSize(size)

    const maxDim = Math.max(size.x, size.y, size.z)
    if (maxDim === 0) return

    const fov = this.camera.fov * (Math.PI / 180)
    const distance = Math.abs(maxDim / (2 * Math.tan(fov / 2))) * CAMERA_DISTANCE_MULTIPLIER

    const dir = DEFAULT_CAMERA_DIRECTION.clone().normalize()
    this.camera.position.copy(center).addScaledVector(dir, distance)
    this.camera.near = distance * NEAR_PLANE_FACTOR
    this.camera.far = distance * FAR_PLANE_FACTOR
    this.camera.updateProjectionMatrix()
    this.camera.lookAt(center)
    this.controls.target.copy(center)
    this.controls.update()
  }

  /** 销毁场景管理器 */
  dispose() {
    if (this.animationId !== null) cancelAnimationFrame(this.animationId)
    this.controls.dispose()
    this.renderer.dispose()
    if (this.renderer.domElement.parentElement) {
      this.renderer.domElement.parentElement.removeChild(this.renderer.domElement)
    }
  }
}
