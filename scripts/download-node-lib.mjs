/**
 * download-node-lib.mjs
 *
 * 自动将 node.lib 下载到项目根目录，供 CMake 编译相机插件时链接。
 * Auto-downloads node.lib to the project root for use by CMake when building the camera addon.
 *
 * 使用方法 / Usage:
 *   npm run download:nodelib
 *
 * 无需管理员权限——文件保存到项目根目录，CMakeLists.txt 会自动找到它。
 * No admin privileges required — the file is saved to the project root and
 * CMakeLists.txt will pick it up automatically.
 */

import https from 'node:https';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

// node.lib only exists for Windows x64/x86 builds.
if (process.platform !== 'win32') {
    console.error(`✗ このスクリプトは Windows 専用です。`);
    console.error(`✗ node.lib is only available on Windows. Current platform: ${process.platform}`);
    process.exit(1);
}

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(__dirname, '..');
const version = process.versions.node;
const url = `https://nodejs.org/dist/v${version}/node.lib`;
const destInProject = path.join(projectRoot, 'node.lib');
const destInNodeDir = path.join(path.dirname(process.execPath), 'node.lib');

console.log(`Node.js version : ${version}`);
console.log(`Download URL    : ${url}`);
console.log(`Saving to       : ${destInProject}`);
console.log('');

/**
 * Pipe an HTTP(S) response into a writable file stream, resolving when done.
 * @param {import('node:http').IncomingMessage} res
 * @param {string} targetPath
 * @returns {Promise<void>}
 */
function pipeResponseToFile(res, targetPath) {
    return new Promise((resolve, reject) => {
        const file = fs.createWriteStream(targetPath);
        res.pipe(file);
        file.on('finish', () => {
            file.close((err) => {
                if (err) {
                    // Best-effort cleanup — file may not have been fully written.
                    try { fs.unlinkSync(targetPath); } catch { /* file may not exist yet */ }
                    reject(err);
                } else {
                    resolve();
                }
            });
        });
        file.on('error', (err) => {
            file.close(() => {
                // Best-effort cleanup — file may not have been created yet if error fired early.
                try { fs.unlinkSync(targetPath); } catch { /* file may not exist yet */ }
                reject(err);
            });
        });
    });
}

/**
 * Download a URL to a local file, following one level of redirects.
 * @param {string} srcUrl
 * @param {string} targetPath
 * @returns {Promise<void>}
 */
function downloadFile(srcUrl, targetPath) {
    return new Promise((resolve, reject) => {
        https.get(srcUrl, async (res) => {
            try {
                if (res.statusCode === 301 || res.statusCode === 302) {
                    const location = res.headers.location;
                    if (!location) {
                        return reject(new Error('Redirect with no Location header'));
                    }
                    // Consume and discard the redirect response body to free the socket.
                    res.resume();
                    await new Promise((resolveRedirect, rejectRedirect) => {
                        https.get(location, async (res2) => {
                            try {
                                if (res2.statusCode !== 200) {
                                    res2.resume();
                                    return rejectRedirect(new Error(`HTTP ${res2.statusCode} from ${location}`));
                                }
                                await pipeResponseToFile(res2, targetPath);
                                resolveRedirect();
                            } catch (err) { rejectRedirect(err); }
                        }).on('error', rejectRedirect);
                    });
                    return resolve();
                }
                if (res.statusCode !== 200) {
                    res.resume();
                    return reject(new Error(`HTTP ${res.statusCode} from ${srcUrl}`));
                }
                await pipeResponseToFile(res, targetPath);
                resolve();
            } catch (err) { reject(err); }
        }).on('error', reject);
    });
}

try {
    await downloadFile(url, destInProject);

    const sizeKb = Math.round(fs.statSync(destInProject).size / 1024);
    console.log(`✓ node.lib 已下载到项目根目录 (${sizeKb} KB)`);
    console.log(`  ${destInProject}`);
    console.log('');

    // Best-effort: also copy to the Node.js install directory so future builds
    // that don't use the project-root fallback also work.
    try {
        fs.copyFileSync(destInProject, destInNodeDir);
        console.log(`✓ 同时复制到 Node.js 安装目录：`);
        console.log(`  ${destInNodeDir}`);
    } catch {
        // Copying to C:\Program Files\nodejs\ typically requires admin privileges.
        // This is non-fatal: CMakeLists.txt will find node.lib in the project root.
        console.log(`ℹ 未能复制到 Node.js 安装目录（可能需要管理员权限，可忽略此提示）：`);
        console.log(`  ${destInNodeDir}`);
    }

    console.log('');
    console.log('现在可以运行 npm run build:camera 编译相机插件。');
    console.log('You can now run   npm run build:camera   to build the camera addon.');
} catch (err) {
    console.error(`✗ 下载失败: ${err.message}`);
    console.error('');
    console.error('请手动下载 node.lib：');
    console.error(`  URL  : ${url}`);
    console.error(`  保存到: ${destInProject}`);
    process.exit(1);
}
