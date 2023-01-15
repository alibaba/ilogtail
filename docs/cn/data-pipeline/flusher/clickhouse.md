# ClickHouse

## 简介

`flusher_clickhouse` `flusher`插件可以实现将采集到的数据，经过处理后，发送到 ClickHouse。

## 配置参数

| 参数                                | 类型       | 是否必选 | 说明                                          |
|-----------------------------------|----------|------|---------------------------------------------|
| Addrs                             | String数组 | 是    | ClickHouse 地址                               |
| Debug                             | Boolean     | 否    | 是否开启 ClickHouse-go 调试日志                     |
| Authentication.PlainText.Username | String   | 否    | ClickHouse 用户名                              |
| Authentication.PlainText.Password | String   | 否    | ClickHouse 密码                               |
| Authentication.PlainText.Database | String   | 是    | 插入数据目标数据库名称                                 |
| Cluster                           | String   | 否    | 数据库对应集群名称                                   |
| Table                             | String   | 是    | 插入数据目标 null engine 数据表名称                    |
| MaxExecutionTime                  | Int      | 否    | 单次请求最长执行时间，默认 60 秒                          |
| DialTimeout                       | String   | 否    | Dial 超时时间，默认 10 秒                           |
| MaxOpenConns                      | Int      | 否    | 最大连接数，默认 5                                  |
| MaxIdleConns                      | Int      | 否    | 连接池连接数，默认 5                                 |
| ConnMaxLifetime                   | String   | 否    | 连接维持最大时长，默认 10 分钟                           |
| BufferNumLayers                   | Int   | 否    | Buffer 缓冲区数量，默认 16                          |
| BufferMinTime                     | Int   | 否    | 缓冲区数据刷新限制条件 min_time，默认 10                  |
| BufferMaxTime                     | Int   | 否    | 缓冲区数据刷新限制条件 max_time，默认 100                 |
| BufferMinRows                     | Int   | 否    | 缓冲区数据刷新限制条件 min_rows，默认 10000               |
| BufferMaxRows                     | Int   | 否    | 缓冲区数据刷新限制条件 max_rows，默认 1000000             |
| BufferMinBytes                    | Int   | 否    | 缓冲区数据刷新限制条件 min_bytes，默认 10000000           |
| BufferMaxBytes                    | Int   | 否    | 缓冲区数据刷新限制条件 max_bytes，默认 100000000          |
| Compression                       | String   | 否    | 压缩方式，默认 lz4，可选 none/gzip/deflate/lz4/brzstd |

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
    Authentication:
      PlainText:
        Database: default
        Username: user
        Password: 123456
    Table: demo
```
