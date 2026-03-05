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

## 安装与构建 / Install & Build

详细步骤见 [deploy.txt](./deploy.txt)，概要如下：

```bash
# 1. 安装依赖
npm install
npm install node-addon-api node-api-headers

# 2. 编译相机插件
#    前提：node.exe 同目录下必须存在 node.lib
#    官方 Node.js 安装程序会自动放置该文件。若缺失，运行以下命令查看版本：
#      node -p "process.versions.node"
#    然后从 https://nodejs.org/dist/v{version}/node.lib 下载并放到 node.exe 所在目录
npm run build:camera

# 3. 编译前端
npm run build

# 4. 启动开发模式
npm run dev
```