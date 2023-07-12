# C++ glog日志

## 提供者

[`jyf111`](https://github.com/jyf111)

## 描述

`glog`是Google的一个C++日志库，提供流式风格的API。它的默认输出格式如下所示：

```
<Severity><Date Hour:Minute:Second.microsecond> <ThreadID><SourceFileName>:<LineNo>] <LogMessage>
```

使用`processor_regex`插件，通过正则匹配的方式提取日志中的各个字段。

如果`<LogMessage>`存在换行的情况，还需要首先使用`processor_split_log_regex`插件实现多行日志的正确切分。

## 日志输入样例

```
W20220926 21:37:44.070065 3276269 main.cpp:25] warning message
with multi-line
I20220926 21:37:44.070065 3276269 main.cpp:26] info message
```

## 日志输出样例

```json
{
    "__tag__:__path__": "/logs/glog.log",
    "Severity": "W",
    "Date": "20220926",
    "Time": "21:37:44.070065",
    "ThreadID": "3276269",
    "SoureFileName": "main.cpp",
    "LineNo": "25",
    "Message": "warning message\nwith multi-line",
    "__time__": "1688796869"
}

{
    "__tag__:__path__": "/logs/glog.log",
    "Severity": "I",
    "Date": "20220926",
    "Time": "21:37:44.070065",
    "ThreadID": "3276269",
    "SoureFileName": "main.cpp",
    "LineNo": "26",
    "Message": "info message",
    "__time__": "1688796869"
}
```

日志文本被解析成`Json`对象，各个字段中记录了其中的关键信息，并且可以处理换行的日志信息。

## 采集配置

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /logs # log directory
    FilePattern: glog.log # log file
processors:
  - Type: processor_split_log_regex
    SplitRegex: ([IWEF])(.+)
    SplitKey: content
    PreserveOthers: true
  - Type: processor_regex
    SourceKey: content
    Regex: ([IWEF])(\d{8}) (\d{2}:\d{2}:\d{2}\.\d+) (\d+) (.+):(\d+)] (.+)
    Keys:
      - Severity
      - Date
      - Time
      - ThreadID
      - SoureFileName
      - LineNo
      - Message
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```
