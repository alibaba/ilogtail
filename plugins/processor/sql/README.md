# SQL 处理插件

该插件可以处理结构化日志，使用selection clause筛选列，where clause筛选行，同时支持as重命名列，以及标量函数对列进行处理。

## 参数说明

插件类型（type）为 `processor_sql`。

|参数|类型|必选或可选|参数说明|
|----|----|----|----|
|Script|string|必选|sql脚本命令。|
|NoKeyError|bool|可选|无匹配的key是否记录，默认true。|

支持以下标量函数,且函数参数及语义与MySQL基本一致：

```sql
COALESCE( arg1 <any type>, arg2 <any type> [, ....] )
CONCAT()
CONCAT_WS()
LTRIM(string <STRING>)
RTRIM(string <STRING>)
TRIM(string <STRING>)
LOWER
UPPER
SUBSTRING()
SUBSTRING_INDEX()
LENGTH()
LOCATE()
LEFT()
RIGHT()
REGEXP_INSTR()
REGEXP_LIKE()
REGEXP_REPLACE()
REGEXP_SUBSTR()
REPLACE()
MD5()
SHA1()
SHA2()
TO_BASE64() 
AES_ENCRYPT()(该函数目前是由已有插件改造适配，与MySQL语法略有差异)
IF()
```

支持CASE语句：

```sql
CASE WHEN condition THEN result [WHEN ...] [ELSE result] END
CASE expr WHEN expr1 THEN result [WHEN ...] [ELSE result] END
```

在 MySQL 中，SELECT 语句不支持 IF-THEN-ELSE 条件语句。可以用IF(a, b, c)函数来实现类似的功能。

支持以下操作符：

```sql
<
>
=
!=
REGEXP
LIKE
()
NOT
!
AND
OR
```

sql语法可以参考processor_sql_test.go,如：

```sql
select CASE event_type WHEN 'js_error'  THEN upper('js') WHEN 'perf' THEN upper('system') ELSE upper('java') END file_type from log where event_type='js_error'
select idfa, if(md5(idfa),1,2) if_result from log where action NOT REGEXP 'cli.{2}' or (element LIKE '#Bu_to_' and timestamp LIKE '123456%')
select CASE WHEN regexp_substr(user_agent,'.* ',4) REGEXP '.*/5.0.*' THEN '5' ELSE '4' END version from log where length(timestamp) > 5
```

## 示例

1. ​过滤出关心的js\_error日志 2. ​将timestamp和nanosecond合并为event\_time 3. ​idfa需要数据脱敏 4. 根据​ua提取操作系统 ​5. 对element进行小写归一化​，配置详情及处理结果如下：

- 输入

```json
{"timestamp": 1234567890, "nanosecond": 123456789, "event_type": "js_error", "idfa": "abcdefg", "user_agent": "Chrome on iOS. Mozilla/5.0 (iPhone; CPU iPhone OS 16_5_1 like Mac OS X)", "action": "click", "element": "#Button"}
{"timestamp": 1234567890, "nanosecond": 123456789, "event_type": "perf", "idfa": "abcdefg", "user_agent": "Chrome on iOS. Mozilla/5.0 (iPhone; CPU iPhone OS 16_5_1 like Mac OS X)", "load": 3, "render": 2}
```

- 配置详情

```yaml
enable: true
global:
  StructureType: v2
inputs:
  - Type: input_file
    FilePaths:
      - ./jsonInput.log
processors:
  - Type: processor_json
    SourceKey: content
    KeepSource: false
    ExpandDepth: 1
    ExpandConnector: ""
  - Type: processor_sql
    Script: select concat_ws(".", timestamp, nanosecond) event_time, event_type, md5(idfa) idfa, CASE WHEN user_agent REGEXP ".*iPhone OS.*" THEN "ios" ELSE "android" END os, action, lower(element) element from log where event_type="js_error"
    NoKeyError: true
flushers:
  - Type: flusher_stdout
    FileName: ./out.log
```

- 配置后结果

```json
{
  "event_time": "1234567890.123456789",
  "event_type": "js_error",
  "idfa":       "7ac66c0f148de9519b8bd264312c4d64",
  "os":         "ios",
  "action":     "click",
  "element":    "#button",
}
```
