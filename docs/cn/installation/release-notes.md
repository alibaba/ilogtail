# 发布历史

# 发布历史

## 2.0.7

### 发布记录

发版日期：2024 年 7 月 31 日

问题修复

- [public] [both] [fixed] fix: JSON truncation caused by escaped zero byte (#1594) (#1596)
- [public] [both] [fixed] fix core caused by concurrent use of non-thread-safe gethostbyname (#1611)
- [public] [both] [fixed] Fix issue that guage metric miss labels (#1618)
- [public] [both] [fixed] recover readers exactly from checkpoint (#1620) (#1635)
- [public] [both] [fixed] fix: GTID Truncation Issue and Improve Consistency in Checkpoint Management (#1648)

* 修复转义零字节导致 JSON 截断 [#1596](https://github.com/alibaba/ilogtail/pull/1596)
* 修复使用非线程安全的gethostbyname方法导致的coredump问题 [#1611](https://github.com/alibaba/ilogtail/pull/1611)
* 修复opentelemetry解析guage类型指标数据的时候缺失标签的问题 [#1618](https://github.com/alibaba/ilogtail/pull/1618)
* 修复从checkpoint恢复的时候，轮转文件过多可能导致超出reader队列长度的reader恢复失败，进一步引发在inode复用时，新的reader读到了错误的老reader的checkpoint，会导致截断和重复采集 [#1635, #1638](https://github.com/alibaba/ilogtail/pull/1635, https://github.com/alibaba/ilogtail/pull/1638)
* 修复input_canel插件GTID不准确的问题 [#1648](https://github.com/alibaba/ilogtail/pull/1648)


[详情和源代码](https://github.com/alibaba/ilogtail/blob/main/changes/v2.0.7.md)

### 下载

| 文件名                                                                                                                                          | 系统    | 架构     | SHA256 校验码                                                       |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-2.0.7.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.7/ilogtail-2.0.7.linux-amd64.tar.gz) | Linux | x86-64 | 144b20e4dd4e20b517e49c04c745d2f6a64747ab40c0f4aefc7dcadad5803104 |
| [ilogtail-2.0.7.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.7/ilogtail-2.0.7.linux-arm64.tar.gz) | Linux | arm64  | 4a54a7b11fb4cae0cea17f0f9bfbff95dea4a2163e9a925d6728ef8c8d720392 |

### Docker 镜像

**Docker Pull 命令**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:2.0.7
```


## 2.0.4

### 发布记录

发版日期：2024 年 5 月 16 日

优化

* 发送模块日志优化 [#1359](https://github.com/alibaba/ilogtail/pull/1359)
* 文件删除时，最后一行读取之后等待flush超时而不是立即发送 [#1418](https://github.com/alibaba/ilogtail/pull/1418)
* 最后一行匹配逻辑重构 [#1426](https://github.com/alibaba/ilogtail/pull/1426)
* 增加缺失年份信息的时间格式的测试用例 [#1432](https://github.com/alibaba/ilogtail/pull/1432)
* 聊天群组二维码图片替换 [#1445](https://github.com/alibaba/ilogtail/pull/1445)
* SourceBuffer数据结构支持move [#1457](https://github.com/alibaba/ilogtail/pull/1457)
* 开源之夏2024活动信息 [#1461](https://github.com/alibaba/ilogtail/pull/1461)
* SPL功能开关默认开启 [#1475](https://github.com/alibaba/ilogtail/pull/1475)

问题修复

* 修复dockershim文件存在的情况下,containerd运行时误判的问题 [#1424](https://github.com/alibaba/ilogtail/pull/1424)
* 修复Golang插件模块Pipeline版本识别错误问题 [#1427](https://github.com/alibaba/ilogtail/pull/1427)
* 修复多行分割状态错误问题 [#1410](https://github.com/alibaba/ilogtail/pull/1410)
* 基础镜像更新，修复部分旧CPU型号环境下出现的非法指令集问题 [#1429](https://github.com/alibaba/ilogtail/pull/1429)
* 修复Golang插件模块与C++核心模块采集配置名不一致问题 [#1437](https://github.com/alibaba/ilogtail/pull/1437)
* 修复发送请求中UserAgent信息丢失问题 [#1444](https://github.com/alibaba/ilogtail/pull/1444)
* 修复Golang插件模块告警信息版本不一致问题 [#1443](https://github.com/alibaba/ilogtail/pull/1443)
* 修复LOGTAIL_LOG_LEVEL环境变量配置对ilogtail.LOG不生效的问题 [#1440](https://github.com/alibaba/ilogtail/pull/1440)
* 修复容器信息预览功能开关字段错误问题 [#1430](https://github.com/alibaba/ilogtail/pull/1430)
* 修复iLogtail启动阶段告警信息缺失问题 [#1455](https://github.com/alibaba/ilogtail/pull/1455)
* 修复uuid缺失问题和instance id错误问题 [#1476](https://github.com/alibaba/ilogtail/pull/1476)
* flusher_sls插件允许空endpoint配置 [#1486](https://github.com/alibaba/ilogtail/pull/1486)

文档优化

* flusher_pulsar文档优化 [#1460](https://github.com/alibaba/ilogtail/pull/1460)


[详情和源代码](https://github.com/alibaba/ilogtail/blob/main/changes/v2.0.4.md)

### 下载

| 文件名                                                                                                                                          | 系统    | 架构     | SHA256 校验码                                                       |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-2.0.4.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.4/ilogtail-2.0.4.linux-amd64.tar.gz) | Linux | x86-64 | 144b20e4dd4e20b517e49c04c745d2f6a64747ab40c0f4aefc7dcadad5803104 |
| [ilogtail-2.0.4.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.4/ilogtail-2.0.4.linux-arm64.tar.gz) | Linux | arm64  | 4a54a7b11fb4cae0cea17f0f9bfbff95dea4a2163e9a925d6728ef8c8d720392 |

### Docker 镜像

**Docker Pull 命令**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:2.0.4
```

## 2.0.0

### 发布记录

发版日期：2024 年 1 月 12 日

iLogtail 2.0是一个架构全新升级的版本，与1.x版本的采集配置不是完全兼容的，升级指南请参考变更说明[#1294](https://github.com/alibaba/ilogtail/discussions/1294)

2.0版本的配置方式完美支持了C++原生插件之间的级联和C++原生插件和Go插件之间的级联，同时引入了更加高效的数据处理语言[SLS Processing Language (SPL)](https://help.aliyun.com/zh/sls/user-guide/spl-overview)，端上数据处理更加游刃有余。

新功能

* 支持全新设计的V2采集配置 [#1185](https://github.com/alibaba/ilogtail/pull/1185)
* 支持SPL数据处理语言处理数据 [#1278](https://github.com/alibaba/ilogtail/pull/1278)
* 新增原生Input和Flusher插件接口，并定义完整原生流水线 [#1184](https://github.com/alibaba/ilogtail/pull/1184)
* 即使使用Go插件Tag也将被置于Log的Meta中而非内容中，除非选项UsingOldContentTag为true [#1169](https://github.com/alibaba/ilogtail/pull/1169)
* 输出支持jsonline协议 [#1265](https://github.com/alibaba/ilogtail/pull/1165)
* Env方式控制配置创建升级使用2.0配置管理接口 [#1282](https://github.com/alibaba/ilogtail/pull/1282)

优化

* flusher_http插件新增队列缓存和异步拦截器设置 [#1203](https://github.com/alibaba/ilogtail/pull/1203)
* goprofile插件上报数据中使用机器的IP地址 [#1281](https://github.com/alibaba/ilogtail/pull/1281)
* 增强vscode开发环境体验 [#1219](https://github.com/alibaba/ilogtail/pull/1219)
* kafka输出插件新增MaxOpenRequests选项 [#1224](https://github.com/alibaba/ilogtail/pull/1219)
* 插件自监控指标新增labels支持 [#1240](https://github.com/alibaba/ilogtail/pull/1240)
* loki flusher支持只输出内容 [#1256](https://github.com/alibaba/ilogtail/pull/1256)

问题修复

* 修复K8s集群Pod网络为HostNetwork时获取到的容器IP有时为空的问题 [#1280](https://github.com/alibaba/ilogtail/pull/1280)
* 修复使用libcurl因没有设置CURLOPT_NOSIGNAL导致偶尔崩溃的问题 [#1283](https://github.com/alibaba/ilogtail/pull/1283)
* 修复原生分隔符解析插件解析行首有空格的日志时字段错乱的问题 [#1289](https://github.com/alibaba/ilogtail/pull/1289)
* 修复原生插件丢弃超时日志时区处理错误的问题 [#1293](https://github.com/alibaba/ilogtail/pull/1293)
* 修复解析任意含有content key的json后，原生JSON插件总是错误保留原始content字段的问题 [#1296](https://github.com/alibaba/ilogtail/pull/1296)
* 修复原生分隔符插件的内存泄露问题 [#1300](https://github.com/alibaba/ilogtail/pull/1300)
* 修复因检查点转储早于目录注册导致的日志重复问题 [#1291](https://github.com/alibaba/ilogtail/pull/1291)
* 修复飞天日志无法解析带逗号时间格式的兼容性问题 [#1285](https://github.com/alibaba/ilogtail/pull/1285)
* 如果原生解析失败并选择保留原始日志，原始日志将仅保留在__raw_log__而不再保留在content字段以避免数据重复 [#1304](https://github.com/alibaba/ilogtail/pull/1304)
* 解决了原生飞天解析插件多线程工作时解析错误的问题 [#1305](https://github.com/alibaba/ilogtail/pull/1305)

[详情和源代码](https://github.com/alibaba/ilogtail/blob/main/changes/v2.0.0.md)

### 下载

| 文件名                                                                                                                                          | 系统    | 架构     | SHA256 校验码                                                       |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-2.0.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.0/ilogtail-2.0.0.linux-amd64.tar.gz) | Linux | x86-64 | 0bcd191bc82f1e33d0d4a032ff2c9ea9e75de1dee04f11418107dde9d05b4185 |
| [ilogtail-2.0.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/2.0.0/ilogtail-2.0.0.linux-arm64.tar.gz) | Linux | arm64  | fc825b4879fd1c00bcba94ed19a4484555ced1f9b778f78786bc3e2bfc9ebad8 |

### Docker 镜像

**Docker Pull 命令**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:2.0.0
```

## 1.x版本

[发布记录(1.x版本)](./release-notes-1.md)
