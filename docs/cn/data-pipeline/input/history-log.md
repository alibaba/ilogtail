# 历史文件采集

## 简介

iLogtail只采集增量日志。如果日志文件无更新，则iLogtail不会采集该文件中的日志。如果您需要采集历史日志，可使用iLogtail自带的导入历史日志文件功能。

## 版本

[Stable](../stability-level.md)

## 配置参数

### 基础参数

1. 获取iLogtail配置的唯一标识。
首先，参考[普通文件采集](./file-log.md)，运行iLogtail。在iLogtail运行日志`ilogtail.LOG`中，输出了配置的唯一标识，格式为`config#<配置文件绝对路径>`。该配置文件必须使用`file_log`作为输入插件。

2. 添加本地事件
    1. 在iLogtail安装目录下，创建local_event.json文件。
    2. 在local_event.json文件中添加本地事件，类型为标准JSON，格式如下所示。

```json
[ 
  {
    "config" : "${your_config_unique_id}",
    "dir" : "${your_log_dir}",
    "name" : "${your_log_file_name}",
    "rate": "${your_rate}"
   },
  {
   ...
   }
   ...
]
```

| 参数 | 类型 | 是否必选 | 说明 |
| --- | --- | --- | --- |
| config | String | 是 | 填写步骤1中获取的Logtail配置唯一标识。 |
| dir | String | 是 | 历史日志文件所在目录，例如：/data/logs。注意：1. 文件夹不能以/结尾。2. 文件夹目录不能是iLogtail安装目录（/usr/local/ilogtail）。 |
| name | String | 是 | 历史日志文件名，支持通配符，例如access.log.2018-08-08、access.log*。 |
| rate | Integer | 否 | 日志采集速率（单位：byte），例如：1000000。不填写情况下默认不进行限速。 |
