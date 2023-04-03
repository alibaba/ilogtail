# 正则

## 简介

`processor_regex_replace processor`插件可以通过正则匹配的模式实现文本日志的替换。

## 版本

[Stable](../stability-level.md)

## 配置参数

| 参数           | 类型       | 是否必选 | 说明                                                                        |
| ------------ | -------- | ---- | ------------------------------------------------------------------------- |
| Type         | String   | 是    | 插件类型                                                                      |
| SourceKey    | String   | 是    | 匹配字段名                                                       |
| Regex         | String | 是    | 匹配字段值对应的正则表达式                                       |
| ReplaceString   | String  | 否    | 正则表达式匹配后替换的字符串                               |

## 样例

### 示例 1：基本正则匹配与替换

采集`/home/test-log/`路径下的`regex_replace.log`文件，测试日志内容的正则匹配与替换功能。

* 输入

```bash
echo '2022-09-16 09:03:31.013 \u001b[32mINFO \u001b[0;39m \u001b[34m[TID: N/A]\u001b[0;39m [\u001b[35mThread-30\u001b[0;39m] \u001b[36mc.s.govern.polygonsync.job.BlockTask\u001b[0;39m : 区块采集------结束------\r' >> /home/test-log/regex_replace.log
```

* 采集配置

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test-log/
    FilePattern: regex_replace.log
processors:
  - Type: processor_regex_replace
    SourceKey: content
    Regex: \\u\w+\[\d{1,3};*\d{1,3}m|N/A
    ReplaceString: ''
flushers:
  - Type: flusher_sls
    Endpoint: cn-xxx.log.aliyuncs.com
    ProjectName: test_project
    LogstoreName: test_logstore
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```json
{
    "__tag__:__path__": "/home/test_log/regex_replace.log",
    "content": "2022-09-16 09:03:31.013 INFO  [TID: ] [Thread-30] c.s.govern.polygonsync.job.BlockTask : 区块采集------结束------\r",
    "__time__": "1680353730"
}
```

### 示例 2：根据正则分组匹配与替换

采集`/home/test-log/`路径下的`regex_replace.log`文件，测试日志内容的正则匹配与替换功能。

* 输入

```bash
echo '10.10.239.16' >> /home/test-log/regex_replace.log
```

* 采集配置

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test-log/
    FilePattern: regex_replace.log
processors:
  - Type: processor_regex_replace
    SourceKey: content
    Regex: (\d.*\.)\d+
    ReplaceString: ${1}0/24
flushers:
  - Type: flusher_sls
    Endpoint: cn-xxx.log.aliyuncs.com
    ProjectName: test_project
    LogstoreName: test_logstore
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```json
{
    "__tag__:__path__": "/home/test_log/regex_replace.log",
    "content": "10.10.239.0/24",
    "__time__": "1680353730"
}
```