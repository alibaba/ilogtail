import { createApp } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import App from './App.vue'


let app=createApp(App)
app.use(ElementPlus)
createApp(App).mount('#app')
