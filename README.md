# robot-calibration-tool

## 获取代码 / Get the Code

### 首次克隆（First-time clone）

```bash
git clone https://github.com/huiyingchenpompom/robot-calibration-tool.git
cd robot-calibration-tool
```

> **注意**：协议头必须是 `https://`，不能多一个点（`.https://`），否则会报
> `fatal: protocol '.https' is not supported` 错误。

### 拉取远程分支最新代码（Pull latest from remote branch）

如果已克隆仓库，执行以下命令更新本地代码：

```bash
# 拉取所有远程分支的最新信息
git fetch origin

# 方式一：合并远程分支（推荐，等价于 fetch + merge）
git pull origin <分支名>
# 例如拉取 main 分支：
git pull origin main

# 方式二：检出并跟踪一个远程分支（首次切换到该分支时使用）
git checkout -b <本地分支名> origin/<远程分支名>
# 例如：
git checkout -b copilot/setup-robot-calibration-tool origin/copilot/setup-robot-calibration-tool

# 方式三：已在该分支上，直接拉取
git pull
```

---

## 相机 SDK 支持 / Camera SDK Support

`npm run build:camera` 会在 CMake configure 阶段**自动检测**以下相机 SDK。  
SDK 若未安装，相应相机功能将以存根模式编译（程序可运行，但该品牌相机无法实际连接）。

| 品牌 | SDK | 内置搜索路径（自动检测） |
|------|-----|----------------------|
| Basler | Pylon SDK | `C:/Program Files/Basler/pylon 7/Development/include` |
| 大恒 (Daheng) | Galaxy SDK | `C:/Program Files/Daheng Imaging/GalaxySDK/APIDll/Win64/include`<br>`C:/Program Files/micro-i/sc/camera/inc`<br>`C:/Program Files/micro-i/sc/camera/include`<br>`C:/Program Files/micro-i/sc/camera` |
| 海康 (HIK) | MVS SDK | `C:/Program Files (x86)/MVS/Development/Includes` |

### SDK 安装在自定义路径？使用 local_sdk_paths.cmake

若 SDK 不在上述默认路径（例如安装在其他磁盘或自定义目录），按以下步骤配置：

**第一步**：复制模板文件：

```cmd
cd camera_addon
copy local_sdk_paths.cmake.example local_sdk_paths.cmake
```

**第二步**：用文本编辑器打开 `camera_addon\local_sdk_paths.cmake`，取消注释并填入实际路径：

```cmake
# 示例（大恒 micro-i OEM 安装）:
set(DAHENG_ROOT "C:/Program Files/micro-i/sc/camera")

# 示例（Basler 自定义安装）:
# set(PYLON_ROOT "D:/SDK/Basler/pylon7/Development")

# 示例（海康 MVS 自定义安装）:
# set(HIK_ROOT "D:/SDK/HIK/MVS/Development")
```

**第三步**：重新运行构建即可：

```cmd
npm run build:camera
```

CMake 会打印：
```
-- 已加载本地 SDK 路径配置: .../camera_addon/local_sdk_paths.cmake
-- Daheng Galaxy SDK 已找到: C:/Program Files/micro-i/sc/camera/inc
```

> `local_sdk_paths.cmake` 已加入 `.gitignore`，不会提交到版本库，每台开发机独立维护自己的路径。

---

## 命令行完整编译步骤 / Command-Line Build

### 前置条件（Prerequisites）

| 工具 | 最低版本 | 说明 |
|------|----------|------|
| [Node.js](https://nodejs.org/) | 18 LTS | 无需 `node.lib`——CMake 会自动生成 |
| [CMake](https://cmake.org/download/) | 3.15 | 需加入 PATH |
| Visual Studio Build Tools | 2019 / 2022 | 需勾选"使用 C++ 的桌面开发"工作负载（含 MSVC + Windows SDK） |

> **验证环境**：打开"x64 Native Tools Command Prompt for VS 20xx"或普通命令提示符，运行：
> ```cmd
> node --version
> cmake --version
> cl
> ```
> 三条命令都有输出则环境就绪。

---

### 第一步：进入项目目录

```cmd
cd path\to\robot-calibration-tool
```

---

### 第二步：安装 npm 依赖

> 如果之前安装过但出错，建议先清理再重装：

```cmd
rmdir /s /q node_modules
npm cache clean --force
```

设置镜像（国内网络加速，可选）：

```cmd
set ELECTRON_MIRROR=https://npmmirror.com/mirrors/electron/
set ELECTRON_BUILDER_BINARIES_MIRROR=https://npmmirror.com/mirrors/electron-builder-binaries/
```

安装依赖：

```cmd
npm install
```

> `node-addon-api` 和 `node-api-headers` 已在 `package.json` 的 `dependencies` 中，`npm install` 会自动安装。

---

### 第三步：编译相机插件（camera_addon）

```cmd
npm run build:camera
```

该命令等价于：

```cmd
cd camera_addon
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cd ..
```

> **无需手动处理 `node.lib`。**  
> CMake 会按以下顺序自动找到或生成所需的 `node.lib`：
> 1. Node.js 安装目录（官方安装包标准位置）
> 2. 项目根目录（`npm run download:nodelib` 的下载位置）
> 3. 从 `node-api-headers` 提供的 `.def` 文件使用 `lib.exe` **自动生成**（无需网络、无需管理员权限）

编译成功后输出文件位于：

```
camera_addon\build\Release\camera_addon.node
```

---

### 第四步：编译前端 + 打包 Electron

```cmd
npm run build
```

---

### 第五步：开发模式运行（热更新）

```cmd
npm run dev
```

---

### 常见错误速查

| 错误信息 | 原因 | 解决方法 |
|----------|------|----------|
| `-- Basler/Daheng/HIK SDK 未找到 → 以 stub 模式编译` | **这不是错误**，是 CMake 的 STATUS 提示（`--` 开头表示普通信息）。SDK 未安装时自动进入 stub 模式，`camera_addon.node` 仍会正常生成，连接对应品牌相机时才会在运行时报错。 | 无需处理；如需连接实际相机，安装对应 SDK 并配置 `local_sdk_paths.cmake`（见上方说明） |
| `LNK2019: 无法解析的外部符号 napi_*` | `node.lib` 缺失或生成失败 | 确认已运行 `npm install`；若自动生成失败，运行 `npm run download:nodelib` |
| `C4819` / `C2001 常量中有换行符` | MSVC 以 GBK 解析 UTF-8 源文件 | 已通过 `/utf-8` 编译选项修复，无需手动处理 |
| `protocol '.https' is not supported` | `git clone` URL 前多了一个点 | 使用 `https://` 而非 `.https://` |
| `cmake` 不是内部或外部命令 | CMake 未加入 PATH | 重新安装 CMake 并勾选"Add CMake to the system PATH" |
| `cl` 不是内部或外部命令 | MSVC 环境未激活 | 使用"x64 Native Tools Command Prompt for VS 20xx"运行所有命令 |
| `remove ...app.asar: The process cannot access the file because it is being used by another process` | 上次打包的 Electron 应用仍在运行，Windows 文件被锁 | **关闭所有"Robot Calibration Tool"窗口**，再运行 `npm run build`；若仍失败，运行 `npm run clean` 删除 `release/` 目录后再重试 |