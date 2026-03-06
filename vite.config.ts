import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import electron, { withExternalBuiltins } from 'vite-plugin-electron'
import { builtinModules } from 'node:module'

// 需要外部化的模块（不能被 Vite/Rollup 打包）
const electronExternals = [
  'electron',
  '@grpc/grpc-js',
  '@grpc/proto-loader',
  ...builtinModules.flatMap((m) => [m, `node:${m}`]),
]

export default defineConfig({
  plugins: [
    vue(),
    electron([
      {
        entry: 'src/main/index.ts',
        vite: withExternalBuiltins({
          build: {
            outDir: 'dist-electron',
            rollupOptions: {
              external: electronExternals,
              output: {
                entryFileNames: 'main.js',
              },
            },
          },
        }),
      },
      {
        entry: 'src/main/preload.ts',
        vite: withExternalBuiltins({
          build: {
            outDir: 'dist-electron',
            rollupOptions: {
              external: electronExternals,
            },
          },
        }),
        onstart(options) {
          options.reload()
        },
      },
    ]),
  ],
})
