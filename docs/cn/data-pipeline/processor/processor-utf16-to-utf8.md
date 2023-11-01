# Json

## 简介

`processor_utf16_to_utf8 processor`插件可以将对utf16日志转码成utf8。

## 版本

[Stable](../stability-level.md)

## 配置参数

| 参数     | 类型     | 是否必选 | 说明                                 |
|--------|--------|------|------------------------------------|
| Type   | String | 是    | 插件类型, processor_utf16_to_utf8.     |
| Endian | String | 否    | utf16类型, 可选为Little, Big, 默认为Little |

## 样例

采集`/home/test-log/`路径下的`utf16.log`文件, 文件编码为utf-16LE。

* 输入

```python
import os
import codecs

log_message = "hello world\n"

file_name = "/workspaces/ilogtail_github/bin/log/utf16.log"

if os.path.exists(file_name):
    with open(file_name, 'ab') as f:
        f.write(log_message.encode('utf-16le'))
else:
    with codecs.open(file_name, 'w', 'utf-16-le') as f:
        f.write(log_message)
```

* 采集配置

```yaml
enable: true
global:
  UsingOldContentTag: true
inputs:
  - Type: file_log
    LogPath: /workspaces/ilogtail_github/bin/log
    FilePattern: utf16.log
processors:
  - Type: processor_utf16_to_utf8
    Endian: Little
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```json
{
    "__tag__:__path__": "/home/test_log/utf16.log",
    "content": "hello world",
    "__time__": "1657354602"
}
```
