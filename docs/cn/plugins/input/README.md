# 输入插件

输入插件负责从各种数据源采集数据。LoongCollector 提供两种类型的输入插件:

- 原生插件(C++)
- 扩展插件(Golang)

## 插件类型介绍

### 原生插件

- 采用C++实现，性能最优，资源开销极低
- 专注于常见的数据源采集
- 内置丰富的采集功能和优化逻辑
- 推荐作为首选的采集方案

| 名称                                                                            | 提供方                                                        | 简介                                                    |
|-------------------------------------------------------------------------------|------------------------------------------------------------|-------------------------------------------------------|
| [`input_file`](./input-file.md)<br> 文本日志                                      | SLS官方 | 文本采集。                                                 |
| [`input_container_stdio`](./input-container-stdio.md)<br> 容器标准输出（原生插件）                                      | SLS官方 | 从容器标准输出/标准错误流中采集日志。                                                 |
| [`input_observer_network`](./metric-observer.md)<br>eBPF网络调用数据         | SLS官方                                                      | 支持从网络系统调用中收集四层网络调用，并借助网络解析模块，可以观测七层网络调用细节。 |
| [`input_file_security`](./input-file-security.md)<br> 文件安全数据                                      | SLS官方 | 文件安全数据采集。                                                 |
| [`input_network_observer`](./input-network-observer.md)<br> 网络可观测数据                                      | SLS官方 | 网络可观测数据采集。                                                 |
| [`input_network_security`](./input-network-security.md)<br> 网络安全数据                                      | SLS官方 | 网络安全数据采集。                                                 |
| [`input_process_security`](./input-process-security.md)<br> 进程安全数据                                      | SLS官方 | 进程安全数据采集。                                                 |

### 扩展插件

- 采用Golang实现，性能和资源开销适中
- 支持更多样化的数据源
- 开发门槛低，易于定制开发
- 适用于特殊的数据采集需求

| 名称                                                                            | 提供方                                                        | 简介                                                    |
|-------------------------------------------------------------------------------|------------------------------------------------------------|-------------------------------------------------------|
| [`input_command`](./input-command.md)<br>脚本执行数据                           | 社区<br>[`didachuxing`](https://github.com/didachuxing)      | 采集脚本执行数据。                                             |
| [`input_docker_stdout`](./service-docker-stdout.md)<br>容器标准输出             | SLS官方                                                      | 从容器标准输出/标准错误流中采集日志。                                   |
| [`metric_debug_file`](./metric-debug-file.md)<br>文本日志（debug）              | SLS官方                                                      | 用于调试的读取文件内容的插件。                                       |
| [`metric_input_example`](./metric-input-example.md)<br>MetricInput示例插件    | SLS官方                                                      | MetricInput示例插件。                                      |
| [`metric_meta_host`](./metric-meta-host.md)<br>主机Meta数据                   | SLS官方                                                      | 主机Meta数据。                                             |
| [`metric_mock`](./metric-mock.md)<br>Mock数据-Metric                        | SLS官方                                                      | 生成metric模拟数据的插件。                                      |
| [`metric_system_v2`](./metric-system.md)<br>主机监控数据                        | SLS官方                                                      | 主机监控数据。                                               |
| [`service_canal`](./service-canal.md)<br>MySQL Binlog                     | SLS官方                                                      | 将MySQL Binlog输入到iLogtail。                             |
| [`service_go_profile`](./service-goprofile.md)<br>GO Profile              | SLS官方                                                      | 采集Golang pprof 性能数据。                                  |
| [`service_gpu_metric`](./service-gpu.md)<br>GPU数据                         | SLS官方                                                      | 支持收集英伟达GPU指标。                                         |
| [`service_http_server`](./service-http-server.md)<br>HTTP数据               | SLS官方                                                      | 接收来自unix socket、http/https、tcp的请求，并支持sls协议、otlp等多种协议。 |
| [`service_input_example`](./service-input-example.md)<br>ServiceInput示例插件 | SLS官方                                                      | ServiceInput示例插件。                                     |
| [`service_journal`](./service-journal.md)<br>Journal数据                    | SLS官方                                                      | 从原始的二进制文件中采集Linux系统的Journal（systemd）日志。               |
| [`service_kafka`](./service-kafka.md)<br>Kafka                            | SLS官方                                                      | 将Kafka数据输入到iLogtail。                                  |
| [`service_mock`](./service-mock.md)<br>Mock数据-Service                     | SLS官方                                                      | 生成service模拟数据的插件。                                     |
| [`service_mssql`](./service-mssql.md)<br>SqlServer查询数据                    | SLS官方                                                      | 将Sql Server数据输入到iLogtail。                             |
| [`service_otlp`](./service-otlp.md)<br>OTLP数据                             | 社区<br>[`Zhu Shunjia`](https://github.com/shunjiazhu)       | 通过http/grpc协议，接收OTLP数据。                               |
| [`service_pgsql`](./service-pgsql.md)<br>PostgreSQL查询数据                   | SLS官方                                                      | 将PostgresSQL数据输入到iLogtail。                            |
| [`service_syslog`](./service-syslog.md)<br>Syslog数据                       | SLS官方                                                      | 采集syslog数据。                                           |

## 插件特性对比

| 特性 | 原生插件 | 扩展插件 |
|------|---------|---------|
| 实现语言 | C++ | Golang |
| 性能表现 | 最优 | 适中 |
| 资源开销 | 极低 | 较低 |
| 功能覆盖 | 常见数据源 | 全面覆盖 |
| 开发难度 | 中等 | 较低 |

## 选型建议

1. 采集常见数据源时:
   - 优先使用原生插件
   - 追求极致性能
   - 资源受限场景

2. 以下场景建议使用扩展插件:
   - 需要采集特殊数据源
   - 有定制化需求
   - 需要快速开发
   - 对性能要求不苛刻

## 使用说明

1. 原生Input插件可以同时使用原生Processor插件和扩展Processor插件，或者使用SPL插件;扩展Input插件只能使用扩展Processor插件,具体详见[处理插件](../processor/README.md)

2. 配置示例:
   - [文本日志采集配置](./input-file.md)
   - [容器标准输出采集配置](./input-container-stdio.md)
