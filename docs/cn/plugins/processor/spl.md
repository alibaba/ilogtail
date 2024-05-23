# SPL处理插件

## 简介

`processor_spl`插件使用SPL语言处理数据，支持对数据进行解析、过滤、加工等操作。

随着流式处理的发展，出现了越来越多的工具和语言，使得数据处理变得更加高效、灵活和易用。在此背景下，SLS 推出了 SPL(SLS Processing Language) 语法，以此统一查询、端上处理、数据加工等的语法，保证了数据处理的灵活性。iLogtail 作为日志、时序数据采集器，在 2.0 版本中，全面支持了 SPL 。

### SPL

iLogtail 目前的处理模式一共支持 3 种处理模式。
● 原生插件模式：由 C++实现的原生插件，性能最强。
● 拓展插件模式：由 Go 实现的拓展插件，提供了丰富的生态，足够灵活。
● SPL 模式：随着 iLogtail 2.0 在 C++处理插件中支持了 SPL 的处理能力，对数据处理能力带来了很大的提升，兼顾性能与灵活性。用户只需要编写 SPL 语句，即可以利用 SPL 的计算能力，完成对数据的处理。SPL 语法可以参考[文档](https://help.aliyun.com/zh/sls/user-guide/spl-syntax/?spm=a2c4g.11186623.0.0.2bb67eeaChOwLy)。

![image](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/spl.png)

总的来说，iLogtail + SPL 主要有以下的优势：

1.  统一数据处理语言：对于同样一种格式的数据，用户可以在不同场景中使用同一种语言进行处理，提高了数据处理的效率

2.  查询处理更高效：SPL 对弱结构化数据友好，同时 SPL 主要算子由 C++实现，接近 iLogtail 1.X 版本的原生性能，远高于拓展插件的性能

3.  丰富的工具和函数：SPL 提供了丰富的内置函数和算子，用户可以更加灵活地进行组合

4.  简单易学：SPL 属于一种低代码语言，用户可以快速上手，日志搜索、处理一气呵成

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为processor\_spl。  |
|  Script  |  string  |  是  |  /  |  SPL语句。  |

## 样例

### 正则解析

根据正则提取提取字段

**输入**
```
127.0.0.1 - - [07/Jul/2022:10:43:30 +0800] "POST /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"
```

**处理插件模式**
```
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Regex: ([\d\.]+) \S+ \S+ \[(\S+) \S+\] \"(\w+) ([^\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\"]*)\" \"([^\"]*)\"
    Keys:
    - ip
    - time
    - method
    - url
    - request_time
    - request_length
    - status
    - length
    - ref_url
    - browser
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-regexp content, '([\d\.]+) \S+ \S+ \[(\S+) \S+\] \"(\w+) ([^\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\"]*)\" \"([^\"]*)\"' as ip, time, method, url, request_time, request_length, status, length, ref_url, browser
    | project-away content
```

**输出**
```
{
    "ip": "127.0.0.1",
    "time": "07/Jul/2022:10:43:30",
    "method": "POST",
    "url": "/PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1713184059"
}
```

### 分隔符解析

根据分隔符分隔提取字段

**输入**
```
127.0.0.1,07/Jul/2022:10:43:30 +0800,POST,PutData Category=YunOsAccountOpLog,0.024,18204,200,37,-,aliyun-sdk-java
```

**处理插件模式**
```
processors:
  - Type: processor_parse_delimiter_native
    SourceKey: content
    Separator: ","
    Quote: '"'
    Keys:
    - ip
    - time
    - method
    - url
    - request_time
    - request_length
    - status
    - length
    - ref_url
    - browser
```

SPL模式
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-csv content as ip, time, method, url, request_time, request_length, status, length, ref_url, browser
    | project-away content
```

**输出**
```
{
    "ip": "127.0.0.1",
    "time": "07/Jul/2022:10:43:30 +0800",
    "method": "POST",
    "url": "PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1713231487"
}
```

### Json 解析

解析 json 格式日志

**输入**
```
127.0.0.1,07/Jul/2022:10:43:30 +0800,POST,PutData Category=YunOsAccountOpLog,0.024,18204,200,37,-,aliyun-sdk-java
```

**处理插件模式**
```
processors:
  - Type: processor_parse_json_native
    SourceKey: content
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-json content
    | project-away content
```

**输出**
```
{
    "url": "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1",
    "ip": "10.200.98.220",
    "user-agent": "aliyun-sdk-java",
    "request": "{\"status\":\"200\",\"latency\":\"18204\"}",
    "time": "07/Jul/2022:10:30:28",
    "__time__": "1713237315"
}
```

### 正则解析+时间解析

根据正则表达式解析字段，并将其中的一个字段解析成日志时间

**输入**
```
127.0.0.1,07/Jul/2022:10:43:30 +0800,POST,PutData Category=YunOsAccountOpLog,0.024,18204,200,37,-,aliyun-sdk-java
```

**处理插件模式**
```
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Regex: ([\d\.]+) \S+ \S+ \[(\S+) \S+\] \"(\w+) ([^\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\"]*)\" \"([^\"]*)\"
    Keys:
    - ip
    - time
    - method
    - url
    - request_time
    - request_length
    - status
    - length
    - ref_url
    - browser
  - Type: processor_parse_timestamp_native
    SourceKey: time
    SourceFormat: '%Y-%m-%dT%H:%M:%S'
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    * 
    | parse-regexp content, '([\d\.]+) \S+ \S+ \[(\S+)\] \"(\w+) ([^\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\"]*)\" \"([^\"]*)\"' as ip, time, method, url, request_time, request_length, status, length, ref_url, browser
    | extend ts=date_parse(time, '%Y-%m-%d %H:%i:%S')
    | extend __time__=cast(to_unixtime(ts) as INTEGER)
    | project-away ts
    | project-away content
```

**输出**
```
{
    "ip": "127.0.0.1",
    "time": "07/Jul/2022:10:43:30 +0800",
    "method": "POST",
    "url": "PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1713231487"
}
```

### 正则解析+过滤

根据正则表达式解析字段，并根据解析后的字段值过滤日志

**输入**
```
127.0.0.1 - - [07/Jul/2022:10:43:30 +0800] "POST /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"
127.0.0.1 - - [07/Jul/2022:10:44:30 +0800] "Get /PutData?Category=YunOsAccountOpLog" 0.024 18204 200 37 "-" "aliyun-sdk-java"
```

**处理插件模式**
```
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Regex: ([\d\.]+) \S+ \S+ \[(\S+) \S+\] \"(\w+) ([^\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\"]*)\" \"([^\"]*)\"
    Keys:
    - ip
    - time
    - method
    - url
    - request_time
    - request_length
    - status
    - length
    - ref_url
    - browser
    - Type: processor_filter_regex_native
    FilterKey:
    - method
    - status
    FilterRegex:
    - ^(POST|PUT)$
    - ^200$
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-regexp content, '([\d\.]+) \S+ \S+ \[(\S+) \S+\] \"(\w+) ([^\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\"]*)\" \"([^\"]*)\"' as ip, time, method, url, request_time, request_length, status, length, ref_url, browser
    | project-away content
    | where regexp_like(method, '^(POST|PUT)$') and regexp_like(status, '^200$')
```

**输出**
```
{
    "ip": "127.0.0.1",
    "time": "07/Jul/2022:10:43:30",
    "method": "POST",
    "url": "/PutData?Category=YunOsAccountOpLog",
    "request_time": "0.024",
    "request_length": "18204",
    "status": "200",
    "length": "37",
    "ref_url": "-",
    "browser": "aliyun-sdk-java",
    "__time__": "1713238839"
}
```

### 脱敏

将日志中的敏感信息脱敏

**输入**
```
{"account":"1812213231432969","password":"04a23f38"}
```

**处理插件模式**
```
processors:
  - Type: processor_desensitize_native
    SourceKey: content
    Method: const
    ReplacingString: "******"
    ContentPatternBeforeReplacedString: 'password":"'
    ReplacedContentPattern: '[^"]+'
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-regexp content, 'password":"(\S+)"' as password
    | extend content=replace(content, password, '******')
```

**输出**
```
{
    "content": "{\"account\":\"1812213231432969\",\"password\":\"******\"}",
    "__time__": "1713239305"
}
```

### 添加字段

在输出结果中添加字段

**输入**
```
this is a test log
```

**处理插件模式**
```
processors:
  - Type: processor_add_fields
    Fields:
    service: A
    IgnoreIfExist: false
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | extend service='A'
```

**输出**
```
{
    "content": "this is a test log",
    "service": "A",
    "__time__": "1713240293"
}
```

### Json 解析+丢弃字段

解析 json 并删除解析后的指定字段

**输入**
```
{"key1": 123456, "key2": "abcd"}
```

**处理插件模式**
```
processors:
  - Type: processor_parse_json_native
    SourceKey: content
    - Type: processor_drop
    DropKeys: 
    - key1
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-json content
    | project-away content
    | project-away key1
```

**输出**
```
{
"key2": "abcd",
"__time__": "1713245944"
}
```

### Json 解析+重命名字段

解析 json 并重命名解析后的字段

**输入**
```
{"key1": 123456, "key2": "abcd"}
```

**处理插件模式**
```
processors:
  - Type: processor_parse_json_native
    SourceKey: content
    - Type: processor_rename
    SourceKeys:
    - key1
    DestKeys:
    - new_key1
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-json content
    | project-away content
    | project-rename new_key1=key1
```

**输出**
```
{
    "new_key1": "123456",
    "key2": "abcd",
    "__time__": "1713249130"
}
```

### Json 解析+过滤日志

解析 json 并根据字段条件过滤日志

**输入**
```
{"ip": "10.**.**.**", "method": "POST", "browser": "aliyun-sdk-java"}
{"ip": "10.**.**.**", "method": "POST", "browser": "chrome"}
{"ip": "192.168.**.**", "method": "POST", "browser": "aliyun-sls-ilogtail"}
```

**处理插件模式**
```
processors:
  - Type: processor_parse_json_native
    SourceKey: content
    - Type: processor_filter_regex
    Include:
    ip: "10\..*"
    method: POST
    Exclude:
    browser: "aliyun.*"
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-json content
    | project-away content
    | where regexp_like(ip, '10\..*') and regexp_like(method, 'POST') and not regexp_like(browser, 'aliyun.*')
```

**输出**
```
{
    "ip": "10.**.**.**",
    "method": "POST",
    "browser": "chrome",
    "__time__": "1713246645"
}
```

### Json 解析+字段值映射处理

解析 json 并根据字段值的不同，映射为不同的值

**输入**
```
{"_ip_":"192.168.0.1","Index":"900000003"}
{"_ip_":"255.255.255.255","Index":"3"}
```

**处理插件模式**
```
processors:
  - Type: processor_parse_json_native
    SourceKey: content
    - Type: processor_dict_map
    MapDict:
    "127.0.0.1": "LocalHost-LocalHost"
    "192.168.0.1": "default login"
    SourceKey: "_ip_"
    DestKey: "_processed_ip_"
    Mode: "overwrite"
    HandleMissing": true
    Missing: "Not Detected"
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | parse-json content
    | project-away content
    | extend _processed_ip_= 
    CASE 
    WHEN _ip_ = '127.0.0.1' THEN 'LocalHost-LocalHost' 
    WHEN _ip_ = '192.168.0.1' THEN 'default login' 
    ELSE 'Not Detected'
    END
```

**输出**
```
{
    "_ip_": "192.168.0.1",
    "Index": "900000003",
    "_processed_ip_": "default login",
    "__time__": "1713259557"
}
```

### 字符串替换

替换日志中的指定字符串

**输入**
```
hello,how old are you? nice to meet you
```

**处理插件模式**
```
processors:
  - Type: processor_string_replace
    SourceKey: content
    Method: const
    Match: "how old are you?"
    ReplaceString: ""
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | extend content=replace(content, 'how old are you?', '')
```

**输出**
```
{
    "content": "hello, nice to meet you",
    "__time__": "1713260499"
}
```

### 数据编码与解码

对日志进行 Base64 加密

#### Base64

**输入**
```
this is a test log
```

**处理插件模式**
```
processors:
  - Type: processor_base64_encoding
    SourceKey: content
    NewKey: content1
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | extend content1=to_base64(cast(content as varbinary))
```

**输出**
```
{
    "content": "this is a test log",
    "content1": "dGhpcyBpcyBhIHRlc3QgbG9n",
    "__time__": "1713318724"
}
```

#### MD5

对日志进行 MD5 加密

**输入**
```
hello,how old are you? nice to meet you
```

**处理插件模式**
```
processors:
  - Type: processor_string_replace
    SourceKey: content
    Method: const
    Match: "how old are you?"
    ReplaceString: ""
```

**SPL模式**
```
processors:
  - Type: processor_spl
    Script: |
    *
    | extend content1=lower(to_hex(md5(cast(content as varbinary))))
```

**输出**
```
{
    "content": "this is a test log",
    "content1": "4f3c93e010f366eca78e00dc1ed08984",
    "__time__": "1713319673"
}
```
