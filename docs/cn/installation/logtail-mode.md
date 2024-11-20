# LoongCollector 的 Logtail 兼容模式使用指南

## 版本更新说明

作为 2024 年产品规划的重要组成部分，iLogtail 产品将正式更名为 LoongCollector。此次更新主要包含以下变更：

- 程序二进制文件由 iLogtail 更名为 LoongCollector

- 完整重构了目录结构和配置文件布局

## 新版目录结构

假设LoongCollector安装在`/opt/loongcollector`，以下为LoongCollector新布局结构

```plaintext
/
└── /opt/loongcollector/
                       ├── loongcollector                 # 主程序
                       ├── libPluginAdapter.so
                       ├── libPluginBase.so
                       ├── ca-bundle.crt
                       ├── plugins/                       # 插件目录
                       │      └── custom plugins          # 自定义插件
                       ├── dump                           # 仅由 service_http_server 输入插件使用
                       ├── thirdparty                     # 第三方依赖
                       │      ├── jvm
                       │      └── telegraf
                       ├── conf/                          # 配置目录
                       │      ├── scripts
                       │      ├── apsara_log_conf.json
                       │      ├── plugin_logger.xml
                       │      ├── user_defined_id
                       │      ├── authorization.json
                       │      ├── continuous_pipeline_config/
                       │      │                 ├── local/
                       │      │                 │         └── collect_stdout.json
                       │      │                 └── remote/
                       │      │                           └── collect_file.json
                       │      └── instance_config/
                       │                        ├── local/
                       │                        │         ├── loongcollector_config.json（main configuration）
                       │                        │         └── ebpf.json
                       │                        └── remote/
                       │                                  ├── region.json
                       │                                  └── resource.json
                       ├── data/                                    # 数据目录
                       │       ├── file_check_point                 # 文件采集的checkpoint
                       │       ├── exactly_once_checkpoint/
                       │       ├── go_plugin_checkpoint/            # go插件采集的checkpoint
                       │       ├── docker_path_config.json
                       │       ├── send_buffer_file_xxxxxxxxxxxx
                       │       └── backtrace.dat
                       ├── log/                                     # 日志目录
                       │       ├── loongcollector.log
                       │       ├── loongcollector.log.1
                       │       ├── go_plugin.log
                       │       ├── go_plugin.log.1
                       │       ├── logger_initialization.log
                       │       └── snapshot/
                       └── run/
                               ├── loongcollector.pid
                               ├── inotify_watcher_dirs
                               └── app_info.json
```

## 兼容模式配置说明

为确保现有 Logtail 用户能够平滑升级到 LoongCollector，我们提供了完整的兼容模式支持：

### 主机环境配置

启用兼容模式有两种方式：

1. 命令行参数方式：

```bash
./loongcollector --logtail_mode=true
```

2. 环境变量方式：

```bash
export logtail_mode=true
./loongcollector
```

### 容器环境配置

在容器环境中使用时，需要：

1. 添加环境变量：

```bash
logtail_mode=true
```

2. 调整挂载路径映射：

- 原路径：`/usr/local/ilogtail`
- 新路径：`/usr/local/loongcollector`

示例配置调整：

```plaintext
# 检查点目录
旧路径: /usr/local/ilogtail/checkpoint
新路径: /usr/local/loongcollector/checkpoint

# 配置目录
旧路径: /usr/local/ilogtail/config/local
新路径: /usr/local/loongcollector/config/local
```

通过以上配置，您可以确保现有的 Logtail 配置和数据在升级到 LoongCollector 后能够继续正常运行。
