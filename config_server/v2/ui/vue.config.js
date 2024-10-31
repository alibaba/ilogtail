const { defineConfig } = require('@vue/cli-service')
module.exports = defineConfig({
  transpileDependencies: true,
  lintOnSave: false,
  configureWebpack: {
    resolve: {
      fallback: {
        "util": false,
      }
    }
  },
  devServer: {
    client: {
      overlay: false
    },
    proxy: {
      "api": {
        target: process.env.CONFIG_SERVER,
        changeOrigin: true,
        pathRewrite: {
          "^/api": ""
        }
      }
    }
  }
})
