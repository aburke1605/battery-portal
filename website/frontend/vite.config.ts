import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { viteStaticCopy  } from 'vite-plugin-static-copy';

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    react(),
    viteStaticCopy ({
      targets: [
        {
          src: 'battery.html',
          dest: '',
        },
      ],
    }),
  ],
  base: '/react/', // so the build can be used in flask app
  build: {
    rollupOptions: {
      output: {
        entryFileNames: 'assets/main.js', // so there's no random hash e.g. "index-G8ctO5Ha.js -> main.js"
        assetFileNames: (assetInfo) => {
          if (assetInfo.name?.endsWith('.css')) {
            return 'assets/style.css'; // Fixed CSS filename
          }
          return 'assets/[name].[ext]'; // Default for other assets
        },
      },
    },
  },
})
