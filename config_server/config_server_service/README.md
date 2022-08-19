# ConfigServer

## 代码架构

各模块在源代码中对应的位置：

* [controller/user](controller/user): 用户接口层
* [controller/agent](controller/agent): agent 接口层
* [config_manager](service/config_manager): 配置管理器
* [agent_manager](service/agent_manager): agent 管理器
* [store](store): 存储适配层

其他源代码：

* [common](common): 通用函数
* [router](router): 路由管理
* [setting](setting): 本地设置
* [model](model): 数据结构
* [docs](docs): 文档资料

整体流程大约是：

发送（用户/agent） --> 路由（router） --> 接口（controller user/agent） --> 管理器（config_manager/agent_manager） --> 存储（store）

## 调试相关

``` bash
go build -o ConfigServer
nohup ./ConfigServer > stdout.log 2> stderr.log &
```
