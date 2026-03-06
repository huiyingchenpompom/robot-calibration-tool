// URDF 解析器 - 解析机器人 URDF XML 文件
import * as THREE from 'three'

export interface URDFJoint {
  name: string
  type: string
  parent: string
  child: string
  origin: { xyz: THREE.Vector3; rpy: THREE.Euler }
  axis: THREE.Vector3
  limit?: { lower: number; upper: number; velocity: number; effort: number }
}

export interface URDFLink {
  name: string
  visual?: {
    origin: { xyz: THREE.Vector3; rpy: THREE.Euler }
    geometry: {
      type: 'mesh' | 'box' | 'cylinder' | 'sphere'
      filename?: string
      size?: THREE.Vector3
      radius?: number
      length?: number
    }
    material?: { color?: THREE.Color }
  }
}

export interface URDFRobot {
  name: string
  links: Map<string, URDFLink>
  joints: Map<string, URDFJoint>
}

function parseVector3(str: string): THREE.Vector3 {
  const [x, y, z] = str.trim().split(/\s+/).map(Number)
  return new THREE.Vector3(x ?? 0, y ?? 0, z ?? 0)
}

function parseEuler(str: string): THREE.Euler {
  const [r, p, y] = str.trim().split(/\s+/).map(Number)
  return new THREE.Euler(r ?? 0, p ?? 0, y ?? 0, 'XYZ')
}

function parseOrigin(element: Element) {
  const xyz = element.getAttribute('xyz') ?? '0 0 0'
  const rpy = element.getAttribute('rpy') ?? '0 0 0'
  return {
    xyz: parseVector3(xyz),
    rpy: parseEuler(rpy),
  }
}

/**
 * 解析 URDF XML 字符串
 */
export function parseURDF(urdfContent: string): URDFRobot {
  const parser = new DOMParser()
  const doc = parser.parseFromString(urdfContent, 'text/xml')
  const robotEl = doc.querySelector('robot')
  if (!robotEl) throw new Error('无效的 URDF：未找到 robot 元素')

  const robot: URDFRobot = {
    name: robotEl.getAttribute('name') ?? 'robot',
    links: new Map(),
    joints: new Map(),
  }

  // 解析链接
  robotEl.querySelectorAll('link').forEach(linkEl => {
    const link: URDFLink = { name: linkEl.getAttribute('name') ?? '' }
    const visualEl = linkEl.querySelector('visual')
    if (visualEl) {
      const originEl = visualEl.querySelector('origin')
      const geometryEl = visualEl.querySelector('geometry')
      const materialEl = visualEl.querySelector('material')

      link.visual = {
        origin: originEl ? parseOrigin(originEl) : { xyz: new THREE.Vector3(), rpy: new THREE.Euler() },
        geometry: { type: 'box' },
      }

      if (geometryEl) {
        const meshEl = geometryEl.querySelector('mesh')
        const boxEl = geometryEl.querySelector('box')
        const cylinderEl = geometryEl.querySelector('cylinder')
        if (meshEl) {
          link.visual.geometry = {
            type: 'mesh',
            filename: meshEl.getAttribute('filename') ?? '',
          }
        } else if (boxEl) {
          const sizeStr = boxEl.getAttribute('size') ?? '0.1 0.1 0.1'
          link.visual.geometry = { type: 'box', size: parseVector3(sizeStr) }
        } else if (cylinderEl) {
          link.visual.geometry = {
            type: 'cylinder',
            radius: parseFloat(cylinderEl.getAttribute('radius') ?? '0.05'),
            length: parseFloat(cylinderEl.getAttribute('length') ?? '0.1'),
          }
        }
      }

      if (materialEl) {
        const colorEl = materialEl.querySelector('color')
        if (colorEl) {
          const rgba = colorEl.getAttribute('rgba') ?? '0.8 0.8 0.8 1'
          const [r, g, b] = rgba.split(/\s+/).map(Number)
          link.visual.material = { color: new THREE.Color(r, g, b) }
        }
      }
    }
    robot.links.set(link.name, link)
  })

  // 解析关节
  robotEl.querySelectorAll('joint').forEach(jointEl => {
    const parentEl = jointEl.querySelector('parent')
    const childEl = jointEl.querySelector('child')
    const originEl = jointEl.querySelector('origin')
    const axisEl = jointEl.querySelector('axis')
    const limitEl = jointEl.querySelector('limit')

    if (!parentEl || !childEl) return

    const joint: URDFJoint = {
      name: jointEl.getAttribute('name') ?? '',
      type: jointEl.getAttribute('type') ?? 'fixed',
      parent: parentEl.getAttribute('link') ?? '',
      child: childEl.getAttribute('link') ?? '',
      origin: originEl ? parseOrigin(originEl) : { xyz: new THREE.Vector3(), rpy: new THREE.Euler() },
      axis: axisEl ? parseVector3(axisEl.getAttribute('xyz') ?? '0 0 1') : new THREE.Vector3(0, 0, 1),
    }

    if (limitEl) {
      joint.limit = {
        lower: parseFloat(limitEl.getAttribute('lower') ?? '-3.14'),
        upper: parseFloat(limitEl.getAttribute('upper') ?? '3.14'),
        velocity: parseFloat(limitEl.getAttribute('velocity') ?? '1.0'),
        effort: parseFloat(limitEl.getAttribute('effort') ?? '100'),
      }
    }

    robot.joints.set(joint.name, joint)
  })

  return robot
}
