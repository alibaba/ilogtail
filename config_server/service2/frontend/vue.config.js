const { defineConfig } = require('@vue/cli-service')
module.exports = defineConfig({
  transpileDependencies: true,

  devServer:{
    client: {
      overlay: false // 编译错误时，取消全屏覆盖（建议关掉）
    },
      proxy:{
      "api":{
        target:"http://127.0.0.1:9090",
        changeOrigin:true,
        pathRewrite:{
          "^/api":""
        }
      }
    }
  }
})
