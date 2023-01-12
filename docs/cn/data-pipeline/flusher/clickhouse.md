# ClickHouse

## 简介

`flusher_clickhouse` `flusher`插件可以实现将采集到的数据，经过处理后，发送到 ClickHouse。

## 配置参数

| 参数               | 类型        | 是否必选 | 说明                     |
|------------------|-----------|------|------------------------|
| Addrs            | String数组  | 是    | ClickHouse 地址          |
| Username         | String    | 否    | ClickHouse 用户名         |
| Password         | String    | 否    | ClickHouse 密码          |
| Database         | String    | 是    | 插入数据目标 buffer 表所在数据库名称 |
| Table            | String    | 是    | 插入数据目标 buffer 表名       |
| MaxExecutionTime | Int       | 否    | 单次请求最长执行时间，默认 60 秒     |
| DialTimeout      | String    | 否    | Dial 超时时间，默认 10 秒      |
| MaxOpenConns     | Int       | 否    | 最大连接数，默认 5             |
| MaxIdleConns     | Int       | 否    | 连接池连接数，默认 5            |
| ConnMaxLifetime  | String    | 否    | 连接维持最大时长，默认 10 分钟      |

## 样例

采集`/home/test-log/`路径下的所有文件名匹配`*.log`规则的文件，并将采集结果发送到 ClickHouse。

```yaml
enable: true
inputs:
  - Type: file_log
    LogPath: /home/test_log
    FilePattern: "*.log"
flushers:
  - Type: flusher_clickhouse
    Addrs: 
      - 192.XX.XX.1:9092
      - 192.XX.XX.2:9092
    Username: user
    Password: 123456
    Database: demo
    Table: demo
```
