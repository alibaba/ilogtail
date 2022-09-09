# ConfigServer-service

## 简介

这是日志采集 Agent 管控工具 ConfigServer 的后端部分，提供对服从协议的日志采集 Agent 的管控功能。

## 运行

进入 Config Server 的 `service` 文件夹下

``` bash
nohup ./ConfigServer > stdout.log 2> stderr.log &
```

## 通信协议

服从HTTP/HTTPS协议。

### 通用参数

ip 和 port 为 ConfigServer 的 ip 和接收端口，默认为127.0.0.1和8899。

### API 响应规范

数据的发送及接收均为 protocol buffer 格式，参见源代码中的 [proto](github.com/alibaba/ilogtail/config_server/service/proto) 文件夹（阿尔法版，后续可能发生不兼容变更）。

在调用API接口过程中，若返回HTTP状态码为 200，则表示请求成功，若返回HTTP状态码为其他，例如404等，则表示调用API接口失败。服务端会返回请求响应信息如下。code为响应码，表示请求成功/失败的类型；message 是响应信息，包含详细的请求成功信息/失败说明。

```json
{
  "code" : <Code>,
  "message" : <Message>
}
```

  主要响应码如下所示：

| HTTP状态码 | 响应码 | 说明 |
|-|-|-|
|200 | Accept | 请求成功 |
|400 | AgentGroupAlreadyExist | Agent组已存在 |
|400 | ConfigAlreadyExist | 采集配置已存在 |
|400 | AgentAlreadyExist | Agent已存在 |
|400 | InvalidParameter | 无效的参数 |
|400 | BadRequest | 参数不符合要求 |
|404 | AgentGroupNotExist | Agent组不存在 |
|404 | ConfigNotExist | 采集配置不存在 |
|404 | AgentNotExist | Agent不存在 |
|500 | InternalServerError | 内部服务调用错误 |
|500 | RequestTimeout | 请求超时 |
|503 | ServerBusy | 服务器正忙 |

### 接口说明

以API的形式对外提供统一的管控。总体分为两大类：

* 用户API
  * AgentGroup  创建/修改/删除/查看
  * 采集配置 创建/修改/删除/查看
  * 采集配置与 AgentGroup  绑定/解绑
* Agent API
  * Agent心跳/运行统计/Alarm
  * 采集配置获取

## 开发说明

### 开发

* 添加接口

接口统一添加到 `router` 文件夹下的 `router.go` 文件中，并在 `manger` 和 `interface` 文件夹的相应位置添加实现接口的代码。

* 增加存储适配

若想扩展存储方式，例如实现 `mysql` 存储等，可以在 `store` 文件夹下实现已有的数据库接口 `interface_database` ，并在 `store.go` 的 `newStore` 函数中添加对应的存储类型。

### 编译

进入 Config Server 的 `service` 文件夹下

``` bash
go build -o ConfigServer
```

### 测试

测试文件统一存放在 `test` 文件夹下。

可以通过如下方式查看测试覆盖率：

```bash
go test -cover github.com/alibaba/ilogtail/config_server/service/test -coverpkg github.com/alibaba/ilogtail/config_server/service/... -coverprofile coverage.out -v
go tool cover -func=coverage.out -o coverage.txt
# Visualization results: go tool cover -html=coverage.out -o coverage.html
cat coverage.txt
```
