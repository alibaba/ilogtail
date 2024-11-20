# LoongCollector 的 Logtail 兼容模式使用指南

目录结构改造说明请参考 [LoongCollector 的 目录结构说明](../concepts/loongcollector-dir.md)

## 兼容模式配置说明

为确保现有 Logtail 用户能够平滑升级到 LoongCollector，我们提供了完整的兼容模式支持，在兼容模式下，LoongCollector会按照旧的目录结构进行启动：

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
