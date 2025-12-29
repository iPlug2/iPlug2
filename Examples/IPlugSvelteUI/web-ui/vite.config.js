import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  // Build modes:
  // - default: WebView mode (output to resources/web for native apps)
  // - wasm: EMAudioWorklet mode (output to build-web-em for browser)
  const isWasmBuild = mode === 'wasm';

  return {
    plugins: [svelte()],

    build: {
      outDir: isWasmBuild ? '../build-web-em' : '../resources/web',
      emptyOutDir: !isWasmBuild, // Don't clear WASM files in wasm mode
      rollupOptions: {
        input: isWasmBuild ? 'index-wasm.html' : 'index.html',
        output: {
          // Use predictable names for WASM builds
          entryFileNames: isWasmBuild ? 'assets/index.js' : 'assets/[name]-[hash].js',
          chunkFileNames: isWasmBuild ? 'assets/[name].js' : 'assets/[name]-[hash].js',
          assetFileNames: isWasmBuild ? 'assets/[name][extname]' : 'assets/[name]-[hash][extname]'
        }
      }
    },

    base: './',

    // Dev server with COOP/COEP headers for SharedArrayBuffer
    server: {
      headers: {
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'require-corp'
      },
      // Proxy WASM files from a separate server during development
      proxy: isWasmBuild ? {
        '/scripts': 'http://localhost:8080',
        '/styles': 'http://localhost:8080'
      } : undefined
    },

    // Define build-time constants
    define: {
      __WASM_MODE__: JSON.stringify(isWasmBuild)
    }
  }
})
