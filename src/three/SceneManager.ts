// Three.js 场景管理器
import * as THREE from 'three'
import { OrbitControls } from 'three-stdlib'
import type { SceneSettings, JointAngles, Pose6D } from '../types'

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
  private trajectoryLinesGroup: THREE.Group = new THREE.Group()

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
    this.scene.add(this.trajectoryLinesGroup)

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
    this.trajectoryLinesGroup.clear()
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
