# ConfigServer

## 代码架构

各模块在源代码中对应的位置：

* [controller/user](controller/user): 用户接口层
* [controller/ilogtail](controller/ilogtail): ilogtail 接口层
* [service/config_manager](service/config_manager): 配置管理器
* [service/ilogtail_manager](service/ilogtail_manager): ilogtail 管理器
* [store](store): 存储适配层

其他源代码：

* [common](common): 通用函数
* [router](router): 路由管理
* [setting](setting): 本地设置
* [model](model): 数据结构
* [docs](docs): 文档资料

整体流程大约是：

用户/ilogtail --> 路由（router） --> 接口（controller） --> 管理器（service） --> 存储（store）
