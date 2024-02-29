# Json解析原生处理插件

## 简介

`processor_parse_json_native`插件解析事件中`Json`格式字段内容并提取新字段。

## 版本

[Stable](../stability-level.md)

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为processor\_spl。  |
|  Script  |  string  |  是  |  /  |  spl语句  |
|  TimeoutMilliSeconds  |  int  |  否  |  1000  |  执行脚本的超时时间，单位为毫秒  |
|  MaxMemoryBytes  |  int  |  否  |  50*1024*1024  |  spl执行最大内存配置，单位为字节  |

## 样例

采集文件`/home/test-log/test.log`，解析日志内容并提取字段，并将结果输出到stdout。

- 输入

```
[2024-01-05T12:07:00.123456] {"message": "this is a msg", "level": "INFO", "garbage": "xxx"} java.lang.Exception: exception
```

- 采集配置

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/test.log
    Multiline:
      StartPattern: \[\d+.*
processors:
  - Type: processor_spl
    Script: '* | parse-regexp content, ''\[([^]]+)]\s+([^}]+})\s+([\S\s]*)'' as time,json,stack | parse-json json | project-away garbage,json,content'
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

- 输出

```json
{
    "__tag__:__path__": "/home/test-log/test.log",
    "message": "this is a msg",
    "level": "INFO",
    "time": "2024-01-05T12:07:00.123456",
    "stack": "java.lang.Exception: exception",
    "__time__": "1657161028"
}
```
