# metric_command 插件

## 简介

`metric_command` `metric_command`插件可以通过配置脚本内容，在agent机器上生成可执行的脚本，并通过指定的`cmdpath` 执行该脚本，插件会从脚本执行后`stdout`获得内容进行解析，从而获取脚本内容执行后的信息。


## 版本

[Alpha](../stability-level.md)


## 配置参数

### 基础参数
| 参数 | 类型 | 是否必选 | 说明 |
| --- |  --- | --- | --- |
| Type |String | 是 | 插件类型，指定为`metric_command`。 |
| User |  String | 是 | 执行该脚本内容的用户， 不支持root  |
| ScriptContent|String | 是 | 脚本内容 支持:PlainText|Base64 |
| ScriptType | String | 是 | 指定脚本内容的类型，目前支持:bash, shell, python2, python3|
| ContentEncoding |  String | 否  | 脚本内容的文本格式 <br/> 支持PlainText(纯文本)\|Base64 默认:PlainText|
| ScriptDataDir |  String | 否 | 脚本内容在机器上会被存储为脚本，需要指定存储目录 默认为ilogtail的目录下|
| LineSplitSep |  String | 否 | 脚本输出内容的分隔符 默认:\\n|
| CmdPath | String | 否 | 执行生成脚本的命令路径 默认:/usr/bin/sh|
| TimeoutMilliSeconds | int| 否 | 执行脚本的超时时间  单位为毫秒  默认:3000ms|
| IntervalMs| int| 否 | 采集频率 也是脚本执行的频率 单位为毫秒 默认:5000ms |


* 采集配置

```yaml
enable: true
inputs:
  - Type: metric_command
    User: someone
    ScriptType: shell
    ScriptContent: |+
        echo -e "test metric commond"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```json
{
    "content":"test metric commond",
    "__time__":"1680079323"
}
```


