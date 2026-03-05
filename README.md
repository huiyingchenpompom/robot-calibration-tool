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

## 命令行完整编译步骤 / Command-Line Build

### 前置条件（Prerequisites）

| 工具 | 最低版本 | 说明 |
|------|----------|------|
| [Node.js](https://nodejs.org/) | 18 LTS | 官方安装包会自动在 node.exe 同目录放置 `node.lib` |
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
npm install node-addon-api node-api-headers
```

---

### 第三步：确认 node.lib 存在（Windows 链接器必须）

所有 `napi_*` 符号由 `node.exe` 导出，链接时必须引用 `node.lib`。  
官方 Node.js 安装包会自动放置该文件，但请先确认：

```cmd
REM 查看 node.exe 所在目录
node -p "require('path').dirname(process.execPath)"

REM 检查 node.lib 是否存在（将路径替换为上一命令的输出）
dir "C:\Program Files\nodejs\node.lib"
```

如果 `node.lib` **不存在**，运行以下命令查看 Node.js 版本，然后手动下载：

```cmd
node -p "process.versions.node"
REM 假设版本为 20.11.0，则下载地址为：
REM   https://nodejs.org/dist/v20.11.0/node.lib
REM 下载后放到 node.exe 的同目录，例如：
REM   C:\Program Files\nodejs\node.lib
```

---

### 第四步：编译相机插件（camera_addon）

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

编译成功后输出文件位于：

```
camera_addon\build\Release\camera_addon.node
```

---

### 第五步：编译前端 + 打包 Electron

```cmd
npm run build
```

---

### 第六步：开发模式运行（热更新）

```cmd
npm run dev
```

---

### 常见错误速查

| 错误信息 | 原因 | 解决方法 |
|----------|------|----------|
| `LNK2019: 无法解析的外部符号 napi_*` | `node.lib` 缺失或未链接 | 参见第三步，确认 `node.lib` 存在 |
| `C4819` / `C2001 常量中有换行符` | MSVC 以 GBK 解析 UTF-8 源文件 | 已通过 `/utf-8` 编译选项修复，无需手动处理 |
| `protocol '.https' is not supported` | `git clone` URL 前多了一个点 | 使用 `https://` 而非 `.https://` |
| `cmake` 不是内部或外部命令 | CMake 未加入 PATH | 重新安装 CMake 并勾选"Add CMake to the system PATH" |
| `cl` 不是内部或外部命令 | MSVC 环境未激活 | 使用"x64 Native Tools Command Prompt for VS 20xx"运行所有命令 |