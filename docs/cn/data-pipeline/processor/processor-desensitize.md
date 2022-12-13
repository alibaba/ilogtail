# 数据脱敏

## 简介

`processor_desensitize`插件可以通过正则表达式匹配并实现文本日志中敏感数据的脱敏。[源代码](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/processor_desensitize.go)

## 配置参数

| 参数 | 类型，默认值 | 说明 |
| - | - | - |
| Type                  | String，无默认值(必填) | 插件类型，固定为`processor_desensitize` |
| SourceKey             | String，`content`    | 日志字段名称。 |
| Method                | String，无默认值(必填) | 脱敏方式。可选值如下：<br>const：将敏感内容替换成 ConstString 参数处配置等字符串。<br>md5：将敏感内容替换为其对应的MD5值。 |
| ConstString           | String，无默认值(必填) | 用于替换敏感内容等字符串，当 Method 设置为 const 时，必须配置。 |
| SelectFullField       | Boolean，`true`      | 用于选择脱敏数据的范围。若为true，则对整个字段值进行脱敏；若为false，则对字段值中的指定部分进行脱敏。 |
| RegexBegin            | String，无默认值      | 用于指定敏感内容前缀的正则表达式，当 SelectFullField 配置为 false 时必填。 |
| RegexContent          | String，无默认值      | 用于指定敏感内容的正则表达式，当 SelectFullField 配置为 false 时必填。|
| ReplaceAll            | Boolean，`true`      | 是否对该字段中所有匹配到的敏感内容进行脱敏处理。默认为 true，即替换。若设置为 false，则只对正则表达式匹配到的第一个敏感数据进行脱敏处理。|

## 样例

采集`/home/test-ilogtail/test-log/`路径下包含敏感数据的`processor-desensitize.log`文件，根据指定的配置选项提取日志信息。

* 输入
  
```bash
echo "[{'account':'1812213231432969','password':'04a23f38'}, {account':'1812213685634','password':'123a'}]" >> /home/test-ilogtail/test-log/processor-desensitize.log
```

* 采集配置1

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test-ilogtail/test-log/
    FilePattern: processor-desensitize.log
processors:
  - Type: processor_desensitize
    SourceKey: content
    Method: "const"
    ConstString: "********"
    SelectFullField: true
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出1

```json
{
  "content":"********",
}
```

* 采集配置2

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test-ilogtail/test-log/
    FilePattern: processor-desensitize.log
processors:
  - Type: processor_desensitize
    SourceKey: content
    Method: "const"
    ConstString: "********"
    SelectFullField: false
    RegexBegin: "'password':'"
    RegexContent: "[^']*"
    ReplaceAll: true
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出2

```json
{
  "__tag__:__path__":"/home/test-ilogtail/test-log/processor-desensitize.log",
  "content":"[{'account':'1812213231432969','password':'********'}, {'account':'1812213685634','password':'********'}]",
  "__time__":"1662618045"
}
```

* 采集配置3

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test-ilogtail/test-log/
    FilePattern: processor-desensitize.log
processors:
  - Type: processor_desensitize
    SourceKey: content
    Method: "md5"
    SelectFullField: false
    RegexBegin: "'password':'"
    RegexContent: "[^']*"
    ReplaceAll: false
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出3

```json
{
  "__tag__:__path__":"/home/test-ilogtail/test-log/processor-desensitize.log",
  "content":"[{'account':'1812213231432969','password':'9c525f463ba1c89d6badcd78b2b7bd79'}, {'account':'1812213685634','password':'123a'}]",
  "__time__":"1662618045"
}
```
