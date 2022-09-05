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

整体流程：

用户/agent发起 API 请求 --> 路由（router） --> 接口（user/agent interface） --> 管理器（config/agent manager） --> 存储（store）

## 运行

进入 Config Server 的 service 文件下

``` bash
go build -o ConfigServer
nohup ./ConfigServer > stdout.log 2> stderr.log &
```

## 调试

有多种方式可以调试 API

### 一、【推荐】使用可视化 API 调试工具

* postman

最有名的 API 调试工具之一，但本地测试时，mac 环境出现过链接错误的问题，所以没有使用。

* apizza

比较完善的 API 测试工具。平台上已经搭好了 Config Server 的测试项目，主要的 API 已经配置好，访问后可以直接进行测试。[访问链接](https://www.apizza.net/project/e6fde530057c698049f5be62ad59166d/browse)（密码：`ilogtail`）

### 二、使用控制台 curl 进行调试

#### 采集配置

* 创建采集配置

``` bash
curl --request POST \
  --url http://127.0.01:8899/User/CreateConfig \
  --form configName=flusher_stdout \
  --form 'configInfo=enable: true inputs:   - Type: file_log     LogPath: /home/test-log/     FilePattern: "*.log" flushers:   - Type: flusher_stdout     OnlyStdout: true' \
  --form 'description=实现将采集到的数据，经过处理后，打印到标准输出或者自定义文件。'
```

* 删除采集配置

``` bash
curl --request DELETE \
  --url 'http://127.0.01:8899/User/DeleteConfig?configName=flusher_stdout' 
```

* 修改采集配置

``` bash
curl --request PUT \
  --url http://127.0.01:8899/User/UpdateConfig \
  --form configName=flusher_stdout \
  --form 'configInfo=enable: true inputs:\n  - Type: file_log\n    LogPath: /home/test-log/\n    FilePattern: "*.log"\nflushers:\n  - Type: flusher_stdout\n    OnlyStdout: true\n' \
  --form 'description=实现将采集到的数据，经过处理后，打印到标准输出或者自定义文件。'
```

* 查看采集配置

``` bash
curl --request GET \
  --url 'http://127.0.01:8899/User/GetConfig?configName=flusher_stdout'
```

* 获取所有采集配置

``` bash
curl --request GET \
  --url http://127.0.01:8899/User/ListAllConfigs
```

#### 机器组

* 创建机器组

``` bash
curl --request POST \
  --url http://127.0.01:8899/User/CreateMachineGroup \
  --form groupName=group1 \
  --form groupTag= \
  --form 'description=test machine group'
```

* 删除机器组

``` bash
curl --request DELETE \
  --url 'http://127.0.01:8899/User/DeleteMachineGroup?groupName=group1'
```

* 修改机器组

``` bash
curl --request PUT \
  --url http://127.0.01:8899/User/UpdateMachineGroup \
  --form groupName=group1 \
  --form groupTag=ilogtail \
  --form 'description=test machine group'
```

* 查看机器组

``` bash
curl --request GET \
  --url 'http://127.0.01:8899/User/GetMachineGroup?groupName=group1'
```

* 获取所有机器组

``` bash
curl --request GET \
  --url http://127.0.01:8899/User/ListMachineGroups
```

#### 采集配置与机器组关联

* 机器组与采集配置绑定

```bash
curl --request PUT \
  --url http://127.0.01:8899/User/ApplyConfigToMachineGroup \
  --form groupName=group1 \
  --form configName=flusher_stdout
```

* 机器组与采集配置解绑

```bash
curl --request DELETE \
  --url http://127.0.01:8899/User/RemoveConfigFromMachineGroup \
  --form groupName=group1 \
  --form configName=flusher_stdout
```

* 获取机器组中机器列表

```bash
curl --request GET \
  --url 'http://127.0.01:8899/User/ListMachines?groupName=default'
```

* 查看机器组中的 config 列表

```bash
curl --request GET \
  --url 'http://127.0.01:8899/User/GetAppliedConfigs?groupName=group1'
```

* 查看 config 关联的机器组列表

```bash
curl --request GET \
  --url 'http://127.0.01:8899/User/GetAppliedMachineGroups?configName=flusher_stdout'
```

#### Agent 相关

* Agent 拉取采集配置更新

```bash
curl --request POST \
  --url http://127.0.01:8899/Agent/GetConfigUpdates \
  --form AgentId=ilogtail-1 \
  --form 'configs[old_config]=0'
```

* Agent 发送心跳

```bash
curl --request POST \
  --url 'http://127.0.01:8899/Agent/HeartBeat?AgentId=ilogtail-1' \
  --form AgentId=ilogtail-1 \
  --form 'tags[env]=pre' \
  --form 'tags[app]=nginx'
```

* Agent 发送运行状态

```bash
curl --request POST \
  --url http://127.0.01:8899/Agent/RunningStatus \
  --form AgentId=ilogtail-1 \
  --form 'status[os]=Linux'
```

* Agent 发送告警信息

```bash
curl --request POST \
  --url http://127.0.01:8899/Agent/Alarm \
  --form AgentId=ilogtail-1 \
  --form alarm_type=UnknownType \
  --form alarm_message=message
```
