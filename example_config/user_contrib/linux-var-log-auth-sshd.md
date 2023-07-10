# Linux /var/log/auth.log中的SSH日志

## 提供者
[`shouao.chen`](https://github.com/shouao)

## 描述

`/var/log/auth.log` 是 `Linux` 系统中记录认证日志的文件。`auth.log`文件对于审计和安全分析非常重要，通过分析`auth.log`中的`SSH`日志可以审计用户的`SSH`登录活动，追踪异常访问，查找入侵行为。

## 日志输入样例

```
Jul 10 15:37:27 LabSZ sshd[24200]: Connection closed by 173.234.31.186 [preauth]
Jul 10 15:37:43 LabSZ sshd[24203]: Connection closed by 212.47.254.145 [preauth]
Jul 10 15:37:55 LabSZ sshd[24206]: Invalid user test9 from 52.80.34.196
```

## 日志输出样例

```
2023-07-10 15:37:28 {"__tag__:__path__":"./simple.log","date":"Jul 10","time":"15:37:27","hostname":"LabSZ","pid":"24208","content":"Connection closed by 173.234.31.186 [preauth]Dec 10 06:55:48 LabSZ sshd[24200]: Connection closed by 173.234.31.186 [preauth]","__time__":"1689003474"}
2023-07-10 15:37:43 {"__tag__:__path__":"./simple.log","date":"Jul 10","time":"15:37:43","hostname":"LabSZ","pid":"24203","content":"Connection closed by 212.47.254.145 [preauth]","__time__":"1689003474"}
2023-07-10 15:37:55 {"__tag__:__path__":"./simple.log","date":"Jul 10","time":"15:37:55","hostname":"LabSZ","pid":"24206","content":"Invalid user test9 from 52.80.34.196","__time__":"1689003474"}
```

## 采集配置
```
enable: true
inputs:
  - Type: file_log
    LogPath: /var/log/
    FilePattern: auth.log
processors:
  - Type: processor_regex
    SourceKey: content
    Regex: (\w{3} \d{2}) (\d{2}:\d{2}:\d{2}) (\w+) \S+\[(\d+)\]\S+ (.+)
    Keys:
     - date
     - time
     - hostname
     - pid
     - content
flushers:
  - Type: flusher_sls
    Endpoint: cn-xxx.log.aliyuncs.com
    ProjectName: test_project
    LogstoreName: test_logstore
    Topic: bash_history
  - Type: flusher_stdout
    OnlyStdout: true
```

