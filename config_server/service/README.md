# ConfigServer

## 代码架构

各模块在源代码中对应的位置：

* [interface/user](interface/user): 用户接口层
* [interface/agent](interface/agent): agent 接口层
* [manager/config](manager/config): 配置管理器
* [manager/agent](manager/agent): agent 管理器
* [store](store): 存储适配层

其他源代码：

* [common](common): 通用函数
* [router](router): 路由管理
* [setting](setting): 本地设置
* [model](model): 数据结构
* [docs](docs): 文档资料

整体流程大约是：

用户/agent发起 API 请求 --> 路由（router） --> 接口（user/agent interface） --> 管理器（config/agent manager） --> 存储（store）

## 调试相关

``` bash
go build -o ConfigServer
nohup ./ConfigServer > stdout.log 2> stderr.log &
```
