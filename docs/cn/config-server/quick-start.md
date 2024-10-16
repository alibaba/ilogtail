# 使用介绍

## 一、简介

目前可观测采集 Agent，例如 iLogtail，主要提供了本地采集配置管理模式，当涉及实例数较多时，需要逐个实例进行配置变更，管理比较复杂。此外，多实例场景下 Agent 的版本信息、运行状态等也缺乏统一的监控。因此，需要提供全局管控服务用于对可观测采集 Agent 的采集配置、版本信息、运行状态等进行统一的管理。

ConfigServer 就是这样的一款可观测 Agent 管控工具，目前支持：

* 采集 Agent 注册到 ConfigServer
* 以 Agent 组的形式对采集 Agent 进行统一管理
* 远程批量配置采集 Agent 的采集配置
* 监控采集 Agent 的运行状态
* 监控采集Agent的配置运行状态

## 二、术语表

* 采集 Agent：数据采集器。可以是 iLogtail，或者其他的采集器。
* 采集配置：一个数据采集 Pipeline或Instance，对应一组独立的数据采集配置
* AgentGroup：可以将相同属性的Agent划分为一个组，只需要绑定采集配置到 AgentGroup 即可在组内所有 Agent 生效。
* ConfigServer：采集配置管控的服务端。
* ConfigServer UI：采集配置管控的控制台页面

## 三、功能描述

对采集 Agent 进行全局管控。任何服从协议的Agent，都可以受到统一的管控。

### 3.1 实例注册

* Agent 侧配置已部署的 ConfigServer 信息。
* Agent 启动后，定期向 ConfigServer 进行心跳注册，证明存活性。
* 上报包括但不限于如下的信息，供 ConfigServer 汇集后统一通过API对外呈现。
  * Agent 的 instance_id，作为唯一标识。
  * 版本号
  * 启动时间
  * 运行状态

### 3.2 AgentGroup 管理

* Agent 管理的基本单元是 AgentGroup ，采集配置通过关联 AgentGroup 生效到组内的 Agent 实例上。
* 系统默认创建 default 组，所有 Agent 也会默认加到 default 组。
* 支持通过 AgentGroup 的 Tag 实现自定义分组。如果 Agent 的 Tag 与 AgentGroup 的 Tag 匹配，则加入该组。
* 一个 AgentGroup 可以包含多个 Agent，一个 Agent 可以属于多个组。

### 3.3 采集配置全局管控

* 采集配置通过API进行服务端配置，之后与 AgentGroup 绑定后，即可生效到组内 Agent 实例上。
* 通过采集配置版本号区分采集配置差异，增量变更到 Agent 侧。

### 3.4 状态监控

* Agent 定期向 ConfigServer 发送心跳，上报运行信息。
* ConfigServer 汇集后统一通过API对外呈现。

## 四、运行

ConfigServer 分为 UI 和 Service 两部分，可以分别独立运行，下载ilogtail源码，进入`config_server`目录。

- 后端使用GO基于gin框架开发，针对https://github.com/alibaba/ilogtail/tree/main/config_server/protocol/v2 中提到的Agent行为进行了V2版本的适配，基本实现了能力报告、心跳压缩、配置状态上报等功能，并包含用户端接口，启动见[service](service.md)。
- 前端使用 Vue + Element plus 组件库 + Vue cli 脚手架工具进行开发，旨在为用户提供一个简单、实用、易嵌入的 Config Server 前端控制台，启动见[ui](ui.md)。
- 本项目提供了docker compose一键部署脚本，帮助用户快速搭建可用可视化与agent连通的config-server应用，详情见[deployment](deployment.md)。



