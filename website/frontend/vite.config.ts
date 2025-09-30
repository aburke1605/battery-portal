/// <reference types="node" />
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import svgr from "vite-plugin-svgr";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

export default defineConfig(() => ({
  server: {
    host: true,  // or '0.0.0.0'
    port: 5173,
    proxy: {
      '/api': {
        target: 'https://localhost:5000', // Change this to your backend server URL
        changeOrigin: true,
        secure: false,
      },
    },
  },
  plugins: [
    react(),
    svgr({
      svgrOptions: {
        icon: true,
        exportType: "named",
        namedExport: "ReactComponent",
      },
    }),
  ],
  build: {
    rollupOptions: {
      input: {
        index: path.resolve(__dirname, "index.html"),
        esp: path.resolve(__dirname, "esp32.html"),
      },
      output: {
        // no hash in output file names
        entryFileNames: "assets/[name].js",
        chunkFileNames: "assets/[name].js",
        assetFileNames: "assets/[name].[ext]",
      },
    },
  },
}));
