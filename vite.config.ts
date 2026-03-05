import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import electron from 'vite-plugin-electron'

export default defineConfig({
  plugins: [
    vue(),
    electron([
      {
        entry: 'src/main/index.ts',
      },
      {
        entry: 'src/main/preload.ts',
        onstart(options) {
          options.reload()
        },
      },
    ]),
  ],
})
