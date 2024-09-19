# Config-server

这是基于阿里巴巴 iLogtail 项目 Config Server v2通信协议的一个前后端实现。

后端使用GO基于gin框架开发，针对https://github.com/alibaba/ilogtail/tree/main/config_server/protocol/v2 中提到的Agent行为进行了V2版本的适配，基本实现了能力报告、心跳压缩、配置状态上报等功能，并包含用户端接口，详情见[README.md](service/README.md)。

前端使用 Vue + Element plus 组件库 + Vue cli 脚手架工具进行开发，旨在为用户提供一个简单、实用、易嵌入的 Config Server 前端控制台，详情见[README.md](ui/README.md)。

本项目提供了docker compose一键部署脚本，帮助用户快速搭建可用可视化与agent连通的config-server应用，详情见[README.md](deployment/README.md)。



