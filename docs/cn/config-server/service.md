# Config-server

本项目针对https://github.com/alibaba/ilogtail/tree/main/config_server/protocol/v2 中提到的Agent行为进行了V2版本的适配，基本实现了能力报告、心跳压缩、配置状态上报等功能；并适配用户端实现了Config-server的前端页面Config-server-ui，项目见[ui](ui.md)。

## 快速开始

### 基本配置

下载ilogtail源码，进入`config_server/service`目录

#### 数据库配置

由于要求ConfigServer存储Agent的基本信息、PipelineConfig、InstanceConfig等配置信息，所以首先需要对数据库进行配置。打开`cmd/config/prod`文件夹，编辑`databaseConfig.json`（若不存在，请先创建），填入以下信息，其中`type`为数据库的基本类型 （基于gorm我们适配了`mysql`、`sqlite`、`postgres`、`sqlserver`四种数据库）；其余均为数据库配置信息；`autoMigrate`字段默认为`true`，若先前建好业务相关的表，则设置为`false`。配置完成后，请自行创建可与ConfigServer连通的数据库实例，并创建名字为`dbName`的数据库(若按照docker compose方式启动，无需任何操作，参考[deployment](deployment.md))。

```json
{
  "type": "mysql",
  "userName": "root",
  "password": "123456",
  "host": "mysql",
  "port": 3306,
  "dbName": "test",
  "autoMigrate": true
}
```

#### ConfigServer配置

打开`cmd/config/prod`文件夹，编辑`serverConfig.json`（若不存在，请先创建），填入以下信息，其中`address`为应用程序运行的ip和端口；`capabilities`标识Server端拥有的能力，`responseFlags`支持两种配置采集方式，详情见[protocol](https://github.com/alibaba/ilogtail/blob/main/config_server/protocol/v2/README.md)；若长时间没接收到某个Agent的心跳检测（超时上限为`timeLimit`，单位是秒），server端自动下线该Agent。

```json
{
  "address": "0.0.0.0:9090",
  "capabilities": {
    "rememberAttribute": true,
    "rememberPipelineConfigStatus": true,
    "rememberInstanceConfigStatus": true,
    "rememberCustomCommandStatus": false
  },
  "responseFlags": {
    "fetchPipelineConfigDetail": true,
    "fetchInstanceConfigDetail": false
  },
  "timeLimit":60
}
```

### 运行

按照**基本配置**进行数据库与ConfigServer的配置，进入根目录，运行下面的命令

```shell
go build -o ConfigServer ./cmd

./ConfigServer
```
本项目同样支持docker方式启动应用，Dockerfile见`Dockerfile`（不建议使用，通过docker compose一键部署脚本，可以更快实现应用搭建，参考[deployment](deployment.md)）。
### 启动Agent

Agent 侧需要配置 ConfigServer 信息，才能使用管控功能。从[releases](https://github.com/alibaba/ilogtail/releases) 下载latest版本，配置`ilogtail_config.json`（若不存在，请先创建），添加如下信息，其中`endpoint_list`为ConfigServer的集群地址，并启动Agent。
```json
{
    "config_server_list": [
        {
            "cluster": "community",
            "endpoint_list": [
                "127.0.0.1:9090"
            ]
        }
    ]
}
```
若Agent以docker方式启动（安装与启动见[start-with-container](https://github.com/alibaba/ilogtail/blob/main/docs/cn/installation/start-with-container.md)），务必保证Agent与ConfigServer的网络连通，可选择以下两种配置方案：
* 启动时配置Agent与ConfigServer在同一docker网络`server`中，此时`endpoint_list:["config-server:9090"]`。
* 启动时配置`docker.internal.host`映射，此时`endpoint_list:["docker.internal.host:9090"]`。

