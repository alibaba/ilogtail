# metric_command 插件

## 简介

`metric_command` `metric_command`插件可以通过配置脚本内容，在agent机器上生成可执行的脚本，并通过指定的`cmdpath` 执行该脚本，插件会从脚本执行后`stdout`获得内容进行解析，解析的格式为`sls_metric(可以参考下方)`，从而获取脚本内容执行后的指标的信息，然后通过配置的提交策略收集到指定的指标仓库


## 版本

[Alpha](../stability-level.md)


## 配置参数

### 基础参数
| 参数 | SLS配置参数| 类型 | 是否必选 | 说明 |
| --- | --- | --- | --- | --- |
| Type | type |String | 是 | 插件类型，指定为`metric_command`。 |
| User | user | String | 否 | 执行该脚本内容的用户， 不支持root 默认:nobody |
| ScriptContent|script_content | String | 是 | 脚本内容 支持:PlainText|Base64 |
| ScriptType | script_type| String | 否 | 指定脚本内容的类型，目前支持:bash, shell 默认:bash|
| ContentType | content_type| String | 否  | 脚本内容的文本格式 <br/> 支持PlainText(纯文本)\|Base64 默认:PlainText|
| ScriptDataDir | script_data_dir| String | 否 | 脚本内容在机器上会被存储为脚本，需要指定存储目录 默认为：`/workspaces/ilogtail/scriptStorage/` 建议是在ilogtail的目录下|
| OutputDataType| output_data_type | | String| 否 | 执行脚本后输出stdout的文本格式 默认为:sls_metrics |
| LineSplitSep | line_split_sep | String | 否 | 脚本输出内容的分隔符 默认:\\n|
| ExporterName | exporter_name  |String | 否 | 该脚本执行后生成指标的类别，类似于node_exporter 默认:default |
| CmdPath | cmd_path |String | 否 | 执行生成脚本的命令路径 默认:/usr/bin/sh|
| ExecScriptTimeOut |exec_script_timeout| int| 否 | 执行脚本的超时时间  单位为毫秒  默认:3000ms|
| IntervalMs|interval_ms | int| 否 | 采集频率 也是脚本执行的频率 单位为毫秒 默认:5000ms |



### 参数说明
- User 不支持root
- ContentType 支持脚本内容的文本格式 <br/> 支持PlainText(纯文本)\|Base64 默认:PlainText
- ScriptContent 脚本的内容
**如果脚本文本格式为Base64 需要设置 ContentType:Base64**
- OutputDataType 默认支持sls_metrics

**sls_metrics格式举例**
```
# 这是一个注释
#  __value__:0  __name__:metric_command_example_without_labels
__labels__:b#$#2|a#$#1   __value__:0.0  __name__:metric_command_example  
__value__:0  __name__:metric_command_example_without_labels 
```

- ExporterName 指定后，插件会生成commonLabels同步在上传的指标中
```
script_exporter#$#default //指标采集器的类型
script_md5#$#dafd05fb3b73abcbe44f2536b78c7654 //指标采集器的版本
```

**commonLabels**
```
默认会生成四个公共label
1. hostname
2. ip
3. script_exporter
4. script_md5
```

- ExecScriptTimeOut 脚本执行时间不能大于采集触发的时间

- ScriptDataDir 脚本生成目录, 
**建议是在ilogtail的目录下**
```
脚本生成路径格式: ${ScriptDataDir}/${md5}[0:4]/${md5}[4:8]/${md5}.sh
/workspaces/ilogtail/scriptStorage/dafd/05fb/dafd05fb3b73abcbe44f2536b78c7654.sh
```

## 样例

### 样例1：iLogtail执行脚本内容生成

执行一段可以生成sls_metric格式的的脚本，同时被插件采集到
* 脚本内容

```
`echo -e "__labels__:a#\$#1|b#\$#2    __value__:0  __name__:metric_command_example \n __labels__:a#\$#3|b#\$#4    __value__:3  __name__:metric_command_example2"`

```


* 采集配置

```yaml
enable: true
inputs:
  - Type: metric_command
    ScriptContent: |+
        echo -e "__labels__:a#\$#1|b#\$#2    __value__:0  __name__:metric_command_example \n __labels__:a#\$#3|b#\$#4    __value__:3  __name__:metric_command_example2"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```sls.logs.pb.Log_Content
<Key:"__name__" Value:"metric_command_example" > Contents:<Key:"__labels__" Value:"hostname#$#685081392e57|ip#$#172.17.0.2|script_md5#$#dafd05fb3b73abcbe44f2536b78c7654|script_exporter#$#default" > Contents:<Key:"__time_nano__" Value:"1686794136959055630" > Contents:<Key:"__value__" Value:"0" >
```

### 配置Base64脚本内容
* 采集配置

```yaml
enable: true
inputs:
  - Type: metric_command
    ScriptContent: |+
ZWNobyAtZSAiX19sYWJlbHNfXzphI1wkIzF8YiNcJCMyICAgIF9fdmFsdWVfXzowICBfX25hbWVfXzptZXRyaWNfY29tbWFuZF9leGFtcGxlIFxuIF9fbGFiZWxzX186YSNcJCMzfGIjXCQjNCAgICBfX3ZhbHVlX186MyAgX19uYW1lX186bWV0cmljX2NvbW1hbmRfZXhhbXBsZTIi 
    ContentType:Base64

flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

### 配置脚本执行时间
* 采集配置

```yaml
enable: true
inputs:
  - Type: metric_command
    ScriptContent: |+
ZWNobyAtZSAiX19sYWJlbHNfXzphI1wkIzF8YiNcJCMyICAgIF9fdmFsdWVfXzowICBfX25hbWVfXzptZXRyaWNfY29tbWFuZF9leGFtcGxlIFxuIF9fbGFiZWxzX186YSNcJCMzfGIjXCQjNCAgICBfX3ZhbHVlX186MyAgX19uYW1lX186bWV0cmljX2NvbW1hbmRfZXhhbXBsZTIi 
    ContentType:Base64
    ExecScriptTimeOut: 2000

flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

### 指定用户执行脚本
* 采集配置

```yaml
enable: true
inputs:
  - Type: metric_command
    User: nobody
    ScriptContent: |+
ZWNobyAtZSAiX19sYWJlbHNfXzphI1wkIzF8YiNcJCMyICAgIF9fdmFsdWVfXzowICBfX25hbWVfXzptZXRyaWNfY29tbWFuZF9leGFtcGxlIFxuIF9fbGFiZWxzX186YSNcJCMzfGIjXCQjNCAgICBfX3ZhbHVlX186MyAgX19uYW1lX186bWV0cmljX2NvbW1hbmRfZXhhbXBsZTIi 
    ContentType:Base64

flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```


### SLS JSON配置
* plaintext
```
{
    "inputs": [
        {
            "type": "metric_command",
            "detail": {
                "script_content": "echo -e \"__value__:0.0  __name__:metric_command_example_1 __test__:1 \\n __value__:0.0  __name__:metric_command_example_2   __test__:1 __labels__:b#\\$#2|a#\\$#1 \\n\""
            }
        }
    ],
    "flushers": [
        {
            "type": "flusher_stdout",
            "detail": {
                "FileName": "plaintext.stdout"
            }
        }
    ]
}
```
* plainText 指定export_name
```
{
    "inputs": [
        {
            "type": "metric_command",
            "detail": {
		"exporter_name":"test_exporter",
                "script_content": "echo -e \"__value__:0.0  __name__:metric_command_example_1 __test__:1 \\n __value__:0.0  __name__:metric_command_example_2   __test__:1 __labels__:b#\\$#2|a#\\$#1 \\n\""
            }
        }
    ],
    "flushers": [
        {
            "type": "flusher_stdout",
            "detail": {
                "FileName": "plaintext_exporter.stdout"
            }
        }
    ]
}
```

* 指定执行脚本执行时间
```
{
    "inputs": [
        {
            "type": "metric_command",
            "detail": {
                "script_content": "echo -e \"__value__:0.0  __name__:metric_command_example_1 __test__:1 \\n __value__:0.0  __name__:metric_command_example_2   __test__:1 __labels__:b#\\$#2|a#\\$#1 \\n\"",
                "exec_script_timeout": 2000
            }
        }
    ],
    "flushers": [
        {
            "type": "flusher_stdout",
            "detail": {
                "FileName": "plaintext_timeout.stdout"
            }
        }
    ]
}
```
```
#超时的情况
{
    "inputs": [
        {
            "type": "metric_command",
            "detail": {
                "script_content": " sleep 5 && echo -e \"__value__:0.0  __name__:metric_command_example_1 __test__:1 \\n __value__:0.0  __name__:metric_command_example_2   __test__:1 __labels__:b#\\$#2|a#\\$#1 \\n\"",
                "exec_script_timeout": 2000
            }
        }
    ],
    "flushers": [
        {
            "type": "flusher_stdout",
            "detail": {
                "FileName": "plaintext_timeout_fail.stdout"
            }
        }
    ]
}

```

* 脚本内容base64的情况
```
{
    "inputs": [
        {
            "type": "metric_command",
            "detail": {
                "script_content": "ZWNobyAtZSAiX192YWx1ZV9fOjAuMCAgX19uYW1lX186bWV0cmljX2NvbW1hbmRfZXhhbXBsZV8xIF9fdGVzdF9fOjEgIFxuIF9fdmFsdWVfXzowLjAgIF9fbmFtZV9fOm1ldHJpY19jb21tYW5kX2V4YW1wbGVfMiAgIF9fdGVzdF9fOjEgX19sYWJlbHNfXzpiI1wkIzJ8YSNcJCMxIFxuIg==",
                "content_type": "Base64"
            }
        }
    ],
    "flushers": [
        {
            "type": "flusher_stdout",
            "detail": {
                "FileName": "base64_script_data_dir.stdout"
            }
        }
    ]
}
```

* 指定脚本生成目录的情况
```
{
    "inputs": [
        {
            "type": "metric_command",
            "detail": {
                "script_content": "ZWNobyAtZSAiX192YWx1ZV9fOjAuMCAgX19uYW1lX186bWV0cmljX2NvbW1hbmRfZXhhbXBsZV8xIF9fdGVzdF9fOjEgIFxuIF9fdmFsdWVfXzowLjAgIF9fbmFtZV9fOm1ldHJpY19jb21tYW5kX2V4YW1wbGVfMiAgIF9fdGVzdF9fOjEgX19sYWJlbHNfXzpiI1wkIzJ8YSNcJCMxIFxuIg==",
                "script_data_dir": "/tmp/ilogtail_script",
                "content_type": "Base64"
            }
        }
    ],
    "flushers": [
        {
            "type": "flusher_stdout",
            "detail": {
                "FileName": "base64_script_data_dir.stdout"
            }
        }
    ]
}
```

* 指定用户的配置
```
{
    "inputs": [
        {
            "type": "metric_command",
            "detail": {
                "user":"sa",
                "script_content": "echo -e \"__value__:0.0  __name__:metric_command_example_1 __test__:1 \\n __value__:0.0  __name__:metric_command_example_2   __test__:1 __labels__:b#\\$#2|a#\\$#1 \\n\""
            }
        }
    ],
    "flushers": [
        {
            "type": "flusher_stdout",
            "detail": {
                "FileName": "plaintext_user.stdout"
            }
        }
    ]
}
```


