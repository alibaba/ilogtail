# ilogtail-controller

## 简介

ilogtail-controller 是 ilogtail 开源版的集群管控工具，分为前端的 ilogtail-ui 和后端的 controller。

## 功能

* 提供单机版的 config 管理服务。可以管理百级别机器，千级别采集配置。
* ilogtail 注册到机器组，支持多机器组管理。
* 提供简易的全局管控页面，可以进行全局配置下发。并具备一定的语法校验能力。
* 前端控制台支持心跳、指标监控、运行状态监控。
* 监控数据可扩展对接第三方监控系统。

## 快速启动

* 启动 controller

```bash
cd ./ilogtail/ilogtail-controller/controller
go build
nohup ./controller > controller.log  &
```

## 命令

controller 可以与 ilogtail-ui 和 ilogtail 通信，所有的命令均以 http 请求的形式发出。

### 从 ilogtail-ui 发送到 controller

这里的 ip 和 port 为 controller 的 ip 和接收端口，默认为`127.0.0.1`和`8899`。

#### 获取信息

* `ip:port/fromUI/data/configList/`
  * 功能：获取本地 config 列表
  * 请求方式：GET
  * 参数：无
  * 返回值：
    * http.Status
    * json 数组，为所有本地的 config 信息
* `ip:port/fromUI/data/machineList/`
  * 功能：获取本地机器列表
  * 请求方式：GET
  * 参数：无
  * 返回值：
    * http.Status
    * json 数组，为所有本地注册过的机器及其状态
* `ip:port/fromUI/data/machineGroupList/`
  * 功能：获取本地机器组列表
  * 请求方式：GET
  * 参数：无
  * 返回值：
    * http.Status
    * json 数组，为所有的机器组信息

#### 修改本地信息

* `ip:port/fromUI/local/addConfig/`
  * 功能：导入本地 config
  * 请求方式：POST
  * 参数：json 数据包
    * name：config 名称
    * description：config 描述
    * path：config 文件位置
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/modConfig/`
  * 功能：修改已有的 config 的信息
  * 请求方式：POST
  * 参数：json 数据包
    * name：config 名称
    * description：config 描述
    * path：config 文件位置
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/delConfig/:name`
  * 功能：删除已保存的 config
  * 请求方式：GET
  * 参数：
    * name：要删除的 config 的名字
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/modMachine/`
  * 功能：修改已有的 machine 的信息
  * 请求方式：POST
  * 参数：json 数据包
    * description：machine 描述
    * tag：machine 的标签
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/delMachine/:instance_id`
  * 功能：删除曾经注册但不再活跃的 machine
  * 请求方式：GET
  * 参数：
    * instance_id：要删除的 machine 的唯一标识符
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/addMachineGroup/`
  * 功能：新建机器组
  * 请求方式：POST
  * 参数：json 数据包
    * name：机器组名称
    * description：机器组描述
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/modMachineGroup/`
  * 功能：修改已有的机器组的信息
  * 请求方式：POST
  * 参数：json 数据包
    * name：机器组名称
    * description：机器组描述
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/delMachineGroup/:name`
  * 功能：删除已建立的机器组
  * 请求方式：GET
  * 参数：
    * name：要删除的机器组的名字
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/addMachineToGroup/`
  * 功能：向机器组中批量添加机器
  * 请求方式：POST
  * 参数：
    * name：目标机器组的名字
    * machines：目标机器的 instance_id 数组
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/delMachineFromGroup/`
  * 功能：批量删除机器组中的机器
  * 请求方式：POST
  * 参数：
    * name：目标机器组的名字
    * machines：目标机器的 instance_id 数组
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/addConfigToGroup/`
  * 功能：向机器组中批量添加 config
  * 请求方式：POST
  * 参数：
    * name：目标机器组的名字
    * configs：目标 config 的 name 数组
  * 返回值：
    * http.Status
* `ip:port/fromUI/local/delConfigFromGroup/`
  * 功能：批量删除机器组中的 config
  * 请求方式：POST
  * 参数：
    * name：目标机器组的名字
    * configs：目标 config 的 name 数组
  * 返回值：
    * http.Status

#### 控制 controller 完成命令

* `ip:port/fromUI/toMachine/addConfigToMachine/`
  * 功能：命令 controller 向机器发送 config
  * 请求方式：POST
  * 参数：json
    * configs：config的name数组
    * machines：目标machine的instance_id数组
  * 返回值：
    * http.Status
* `ip:port/fromUI/toMachine/delConfigFromMachine/`
  * 功能：命令 controller 向机器发送删除指定 config 的指令
  * 请求方式：POST
  * 参数：json
    * configs：config的name数组
    * machines：目标machine的instance_id数组
  * 返回值：
    * http.Status

### 从 controller 发送到 ilogtail-ui

这里的 ip 和 port 为目标 ilogtail-ui 的 ip 和接收端口。

暂无

### 从 ilogtail 发送到 controller

这里的 ip 和 port 为 controller 的 ip 和接收端口，默认为`127.0.0.1`和`8899`。

* `ip:port/fromMachine/register/:instance_id`
  * 功能：机器向controller发送注册/心跳
  * 请求方式：GET
  * 参数：
    * instance_id：ilogtail 的唯一标识符
  * 返回值：
    * http.Status
* `ip:port/fromMachine/state/`
  * 功能：机器向 controller 发送自身运行状态
  * 请求方式：POST
  * 参数：json数据包
    * instance_id：ilogtail 的唯一标识符
    * ilogtail 的各种状态
  * 返回值：
    * http.Status
* `ip:port/fromMachine/configList/`
  * 功能：机器向 controller 发送自身已有的 config 列表
  * 请求方式：POST
  * 参数：json数据包
    * configs：已有的 config 的 name 组成的数组
  * 返回值：
    * http.Status

### 从 controller 发送到 ilogtail

这里的 ip 和 port 为目标 ilogtail 的 ip 和接收端口。

* `ip:port/addConfig/`
  * 功能：controller 向 ilogtail 发送 config
  * 请求方式：POST
  * 参数：json数据包
    * name：config 的名称
    * data：config 文件的数据
  * 返回值：
    * http.Status
* `ip:port/delConfig/:name`
  * 功能：controller 命令 ilogtail 删除指定的 config
  * 请求方式：GET
  * 参数：
    * name：config 的名称
  * 返回值：
    * http.Status
