# Release history (version 1.x)

## 1.8.4

### Release records

Release date: January 2, 2024

Optimization

* goprofile the IP address of the machine used in the data reported by the plug-in [#1281](https://github.com/alibaba/ilogtail/pull/1281)

Troubleshooting

* Fix the compatibility problem that time_key is not set to time by default in regular configuration [#1272](https://github.com/alibaba/ilogtail/pull/1272)
* Fix the problem of occasional crash caused by not setting CURLOPT_NOSIGNAL in the use libcurl [#1286](https://github.com/alibaba/ilogtail/pull/1286)
* Fix the problem of field disorder when the native delimiter parsing plug-in parses logs with spaces at the beginning of a row [#1288](https://github.com/alibaba/ilogtail/pull/1288)
* Fix the problem that the native plug-in discards the timeout log time zone processing error [#1293](https://github.com/alibaba/ilogtail/pull/1293)
* Fix the problem that the native json plug-in always keeps the original content key field incorrectly after parsing any JSON containing content [#1295](https://github.com/alibaba/ilogtail/pull/1295)
* Fix the memory leak problem of the native separator plug-in [#1299](https://github.com/alibaba/ilogtail/pull/1299)
* Fix the problem of log duplication caused by checkpoint dump earlier than directory registration [#1301](https://github.com/alibaba/ilogtail/pull/1301)
* Fix the compatibility problem that the Apsara log cannot parse the time format with commas [#1302](https://github.com/alibaba/ilogtail/pull/1302)
* If the native parsing fails and you choose to keep the original log, the original log will only be kept in__raw_log__instead of in the content field to avoid data duplication [#1304](https://github.com/alibaba/ilogtail/pull/1304)
* Fix the problem that the container IP obtained when the Pod network of K8s cluster is HostNetwork is sometimes empty [#1280](https://github.com/alibaba/ilogtail/pull/1280)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.8.4.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.8.4.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.4/ilogtail-1.8.4.Linux-amd64.tar.gz) | Linux | x86-64 | c4b079d01b8dd840f49d4d4387df24c268b4736995c2065a740edd9f2d727356 |
| [ilogtail-1.8.4.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.4/ilogtail-1.8.4.Linux-arm64.tar.gz) | Linux | arm64 | c4c7bb1ccec34cf19424c70f3447121a2dd0285c428bc78308e76ca10348c025 |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.8.4
```

## 1.8.3

### Release records

Release date: December 7, 2023

Troubleshooting

* Fix the problem of repeated log collection introduced by #1216 [#1232](https://github.com/alibaba/ilogtail/pull/1232)
* Fix the plug-in crash caused by container info nil field [#1247](https://github.com/alibaba/ilogtail/pull/1247)
* Fix the problem of ProcessorParseDelimiterNative parsing carrying the next row of data [#1250](https://github.com/alibaba/ilogtail/pull/1250)
* Fix the problem that files may not be read out under the condition of back pressure [#1251](https://github.com/alibaba/ilogtail/pull/1251)
* Fix the problem of plug-in crash caused by plugin_export panic [#1252](https://github.com/alibaba/ilogtail/pull/1252)
* Fix the crash problem caused by parsing logs in Apsara format [#1253](https://github.com/alibaba/ilogtail/pull/1253)
* Fix the problem of data adhesion when parsing logs in Apsara format [#1255](https://github.com/alibaba/ilogtail/pull/1255)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.8.3.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.8.3.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.3/ilogtail-1.8.3.Linux-amd64.tar.gz) | Linux | x86-64 | 1cd352dec783247c4500074f77d8cfb88b607e28f6c95039c8f3da2a7b5880e3 |
| [ilogtail-1.8.3.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.3/ilogtail-1.8.3.Linux-arm64.tar.gz) | Linux | arm64 | 6d77b86ed4b38605ed9e87b3d3ec049da5497b8c48e2cb9ec5334324ef26f0aa |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.8.3
```

## 1.8.1

### Release records

Release date: November 8, 2023

Troubleshooting

* Fix the wrong data sent by Promethus plug-in due to the reuse of string memory [#1201](https://github.com/alibaba/ilogtail/pull/1201)
* Fixed the problem of missing JVM metrics when collecting multiple SkyWalking instances [#1163](https://github.com/alibaba/ilogtail/pull/1163)
* Fix ElasticSearch Flusher TLS authentication problem [#1157](https://github.com/alibaba/ilogtail/issues/1157)
* Fix the type error problem of Profiling under the same Java/Go stack [#1187](https://github.com/alibaba/ilogtail/pull/1187)
* Fix the problem that the local send buffer is lost when upgrading from versions earlier than 1.3 [#1199](https://github.com/alibaba/ilogtail/pull/1199)
* Fix the problem that strptime_ns parses% c differently from the original striptime [#1204](https://github.com/alibaba/ilogtail/pull/1204)
* Fix the problem that topic extraction naming does not support underscores [#1205](https://github.com/alibaba/ilogtail/pull/1205)
* Fix the abnormal status when multiple jmxfetch jmxfetch configured [#1210](https://github.com/alibaba/ilogtail/pull/1210)
* Fix the problem of WSS growth in the collected container memory [#1216](https://github.com/alibaba/ilogtail/pull/1216)
* Fix the problem of incorrect configuration of collection path blacklist without Log prompt [#1218](https://github.com/alibaba/ilogtail/pull/1218)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.8.1.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.8.1.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.1/ilogtail-1.8.1.Linux-amd64.tar.gz) | Linux | x86-64 | b659e711b1960db995787b306c9d87c615345df562affdaa1a090dad7cb453f4 |
| [ilogtail-1.8.1.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.1/ilogtail-1.8.1.Linux-arm64.tar.gz) | Linux | arm64 | a44ef5a4affcbff27dd551c57224c8e8447be37ad6c36292b63790673bba0b7c |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.8.1
```

## 1.8.0

### Release records

Release date: October 10, 2023

New Feature

* C ++ pipeline reconstruction, process plug-in [#995](https://github.com/alibaba/ilogtail/pull/995)
* Multi-line splitting adds the continue/end mode, which supports the combination of multiple modes [#978](https://github.com/alibaba/ilogtail/pull/978)
* Support overtime branch cutting [#1067](https://github.com/alibaba/ilogtail/pull/1067)
* SLS Flusher support nanosecond log time [#1097](https://github.com/alibaba/ilogtail/issues/1097)
* Add Global host path blacklist [#1073](https://github.com/alibaba/ilogtail/pull/1073)
* Add the trace parsing plug-in 'processor_otel_metric '[#1130](https://github.com/alibaba/ilogtail/pull/1130)
* Add the resource label [#1147](https://github.com/alibaba/ilogtail/pull/1147) when creating the collection configuration in the environment variable mode.

Optimization

* Compile the image GCC and upgrade it to 9.3.1 [#1108](https://github.com/alibaba/ilogtail/pull/1108)
* Kafka V2 plug-in supports adding header When writing data [#1065](https://github.com/alibaba/ilogtail/issues/1065)
* Cache logs that do not constitute a complete line, reduce the number of file system calls [#1142](https://github.com/alibaba/ilogtail/pull/1142)
* Support the use of environment variables to control the log printing level [#959](https://github.com/alibaba/ilogtail/issues/959)
* Support the use of flat format when flusher output [#1112](https://github.com/alibaba/ilogtail/pull/1112)
* skywalking plug-in supports capturing 'db. Connection_string' tags [#1131](https://github.com/alibaba/ilogtail/pull/1131)
* Unified formatting of metrics V1 model [#1060](https://github.com/alibaba/ilogtail/pull/1060)
* Verify the NIC IP address to obtain a more accurate host IP address [#1019](https://github.com/alibaba/ilogtail/pull/1019)

Troubleshooting

* Solve the problem of repeated data collection when collecting statefulset with mounted volumes drifting to different nodes [#1081](https://github.com/alibaba/ilogtail/issues/1081)
* Fix the compatibility problem after json processor supports array parsing [#1088](https://github.com/alibaba/ilogtail/issues/1088)
* Fix the problem that the collected log file name is wrong after the empty file inode is reused [#1102](https://github.com/alibaba/ilogtail/issues/1102)
* Fix the problem that the container cannot exit due to checkpoint reopening the file [#1109](https://github.com/alibaba/ilogtail/pull/1109)
* Fix the problem that container stdout cannot be collected when stdout file path is soft chain [#1037](https://github.com/alibaba/ilogtail/issues/1037)
* Fix the problem that the time zone cannot be adjusted for the Apsara log time [#1123](https://github.com/alibaba/ilogtail/pull/1123)
* Fix the problem that the json mode log may not be correctly parsed without carriage return [#1126](https://github.com/alibaba/ilogtail/issues/1126)
* Fix the problem of json parsing exception when the read data contains illegal jon at the beginning [#1161](https://github.com/alibaba/ilogtail/issues/1161)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.8.0.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.8.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.0/ilogtail-1.8.0.Linux-amd64.tar.gz) | Linux | x86-64 | 90f60c372379311880d78a30609e76924f07d1fc8fdbcb944a78c01cc94de891 |
| [ilogtail-1.8.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.8.0/ilogtail-1.8.0.Linux-arm64.tar.gz) | Linux | arm64 | 6737c2f59f4a462d98e46fe5dca8d471a6634531e2cd8a2feb4a6beaaff22d20 |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.8.0
```

## 1.7.1

### Release records

Release date: August 20, 2023

New Feature

* Add the command execution result collection plug-in input_command [#925](https://github.com/alibaba/ilogtail/pull/925)
* 新增log转sls metric插件 processor_log_to_sls_metric [#955](https://github.com/alibaba/ilogtail/pull/955)
* 新增log转sls trace插件 processor_otel_trace [#1028](https://github.com/alibaba/ilogtail/pull/1028)
* Elasticsearch flusher support dynamic index [#979](https://github.com/alibaba/ilogtail/pull/979)
* HTTP Server support accepting original text [#976](https://github.com/alibaba/ilogtail/issues/976)
* Json processor support parsing array [#972](https://github.com/alibaba/ilogtail/pull/972)

Optimization

* Service_prometheus supports automatic scaling in K8s [#932](https://github.com/alibaba/ilogtail/pull/932)
* containerd container collection supports custom rootfs path and automatic discovery path [#985](https://github.com/alibaba/ilogtail/pull/985)
* Nanosecond high precision time performance optimization and windows support [#1026](https://github.com/alibaba/ilogtail/pull/1026)
* Avoid the problem of repeated collection caused by writing files without updating for 30 days [#1039](https://github.com/alibaba/ilogtail/pull/1039)
* Add C ++ Core UT pipeline [#951](https://github.com/alibaba/ilogtail/pull/951)

Troubleshooting

* Fix the problem that service_go_profile may panic [#1036](https://github.com/alibaba/ilogtail/pull/1036)
* Fix zstd batch sending problem [#1006](https://github.com/alibaba/ilogtail/pull/1006)
* Fix the problem that service_otlp plug-in cannot exit [#1040](https://github.com/alibaba/ilogtail/pull/1040)
* Fix the problem that the ttl value is invalid and the empty behavior is inconsistent when env creates the configuration [#1045](https://github.com/alibaba/ilogtail/issues/1045)
* Fix the problem that the container cannot exit due to fd when there is only the container deletion event [#986](https://github.com/alibaba/ilogtail/issues/986)
* Add timeout to the collection disk metrics of system_v2 plug-in to avoid stuck [#933](https://github.com/alibaba/ilogtail/pull/933)
* Fix the problem that the file path data in the same container in the profile overwrites each other [#1034](https://github.com/alibaba/ilogtail/issues/1034)
* Fix the problem of not creating the snapshot directory when the apsara_log_conf.json file exists [#944](https://github.com/alibaba/ilogtail/issues/944)
* Fix the problem that the otlp flusher timestamp is always 1970-01-01 [#1031](https://github.com/alibaba/ilogtail/issues/1031)
* Fix the problem that the config server is sent to delete after the config update [#1023](https://github.com/alibaba/ilogtail/pull/1023)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.7.1.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.7.1.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.7.1/ilogtail-1.7.1.Linux-amd64.tar.gz) | Linux | x86-64 | 13029900e0bdd4f6db858d97b31d30f1659145fc40bfceb97f7946f35c94c232 |
| [ilogtail-1.7.1.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.7.1/ilogtail-1.7.1.Linux-arm64.tar.gz) | Linux | arm64 | fe5e2e2b69cb664ed0fc0578d1fdec95e94ab00711b547c7b90591f0a07853e1 |
| [ilogtail-1.7.1.windows-amd64.zip](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.7.1/ilogtail-1.7.1.windows-amd64.zip)   | Windows | x86-64 | 593f734035f4c808654b638492f1e0d79205b2080a8975305f7f86b988841166 |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.7.1
```

## 1.7.0

### Release records

Release date: July 2, 2023

New Feature

* Writing SLS supports nanosecond high precision time [#952](https://github.com/alibaba/ilogtail/pull/952)

Optimization

* Upgrade the Go version to 1.19 [#936](https://github.com/alibaba/ilogtail/issues/415)
* Add timeout for metric_system_v2 to collect disk indicators to avoid affecting the collection of other indicators [#933](https://github.com/alibaba/ilogtail/pull/933)
* Add the alarm [#915](https://github.com/alibaba/ilogtail/pull/915) of failure of long log splitting.

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.7.0.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.7.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.7.0/ilogtail-1.7.0.Linux-amd64.tar.gz) | Linux | x86-64 | d64061f9b5dc33e40f385c5514000157cc530c5b1f325afb3552267231334cd8 |
| [ilogtail-1.7.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.7.0/ilogtail-1.7.0.Linux-arm64.tar.gz) | Linux | arm64 | 27f9a34df79869bffb1c1be639c3602f1bd021c068e97fc3e287ef9cbabb3834 |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.7.0
```

## 1.6.0

### Release records

Release date: June 8, 2023

New Feature

* Add OpenTelemetry indicator format to convert the SLS indicator format. Support writing the OpenTelemetry indicator data collected by service_otlp and service_http_server plug-ins into the SLS time series Library [#577](https:// github.com/alibaba/ilogtail/issues/577)
* New decoding method of DecodeV2 in InfluxDB format [#846](https://github.com/alibaba/ilogtail/pull/846)
* Pure Go operation mode supports file input to file output test [#869](https://github.com/alibaba/ilogtail/pull/869)
* `metric_meta_kubernetes`插件支持采集[kruise](github.com/openkruise/kruise) CRD元数据 [#878](https://github.com/alibaba/ilogtail/pull/878)
* ConfigServer UI support and protocols use POST instead of GET to transfer data [#894](https://github.com/alibaba/ilogtail/pull/894)
* 'Service-otlp-input 'plug-in supports gzip decompression [#875](https://github.com/alibaba/ilogtail/issues/875)
* Support Prometheus more service discovery [#890](https://github.com/alibaba/ilogtail/pull/890)

Optimization

* Optimize the scene performance of 'fleful_pulsar 'and 'fleful_kafka_v2' under static topics [#824](https://github.com/alibaba/ilogtail/pull/824)
* Upgrade the version of the Kafka client sarama to 1.38.1, and modify the Kafka plug-ins [#600](https://github.com/alibaba/ilogtail/issues/600)
* Optimize the resolution performance of Prometheus protocol about 20% [#866](https://github.com/alibaba/ilogtail/pull/866)
* 优化`processor_desensitize`性能 20-80% [#981](https://github.com/alibaba/ilogtail/pull/891)
* Add ConfigName and tags to the SLS monitoring statistics log to distinguish different configurations from containers [#905](https://github.com/alibaba/ilogtail/pull/905) [#840](https:// github.Com/Alibaba/ilogtail/pull/840)

Troubleshooting

* Fix the problem of abnormal high-frequency access ECS Meta when the network is abnormal [851](https://github.com/alibaba/ilogtail/issues/851)
* Fix the problem that the 'metric_system_v2 'Plug-In collects disk capacity for repeated calculation under the condition of multiple partitions [#835](https://github.com/alibaba/ilogtail/pull/835)
* Fix the problem of application exiting container env collection configuration [#825](https://github.com/alibaba/ilogtail/pull/825)
* Fix the problem that the HTML character is escaped by mistake when the log is converted to JSON Converter [#857](https://github.com/alibaba/ilogtail/pull/857)
* 修复ConfigServer Bug [#876](https://github.com/alibaba/ilogtail/pull/876)
* Fix the problem that containers cannot exit when multiple container file collection configurations point to the same directory [#872](https://github.com/alibaba/ilogtail/issues/872)
* Fix the problem that 'processor_gotime' plug-in cannot express 0 time zone [#829](https://github.com/alibaba/ilogtail/issues/829)
* Fix the problem that the cache of env collection configuration cannot expire [#845](https://github.com/alibaba/ilogtail/issues/845)
* Fix the problem of being discarded when the read start line cannot match in multi-line regular acceleration mode [#902](https://github.com/alibaba/ilogtail/issues/902)
* Fix the problem of redundant carriage return at the end of the line in the multi-line regular mode [#896](https://github.com/alibaba/ilogtail/issues/896)
* Fix the problem that container Label is lost due to file update when obtaining container information through files [#906](https://github.com/alibaba/ilogtail/pull/906)
* Fix the problem of field loss when 'processor_string_replace 'regular mismatch [#892](https://github.com/alibaba/ilogtail/pull/892)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.6.0.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.6.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.6.0/ilogtail-1.6.0.Linux-amd64.tar.gz) | Linux | x86-64 | 392766fc6b2f96df2da4bbd1ffcfc48e2027fc4613138236c7df72dde84d077f |
| [ilogtail-1.6.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.6.0/ilogtail-1.6.0.Linux-arm64.tar.gz) | Linux | arm64 | 50993809b1b741aad4779be7cce3798adb12c1aa8b455538129c15607e186642 |
| [ilogtail-1.6.0.windows-amd64.zip](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.6.0/ilogtail-1.6.0.windows-amd64.zip)   | Windows | x86-64 | f0da617f09ed1db2cd41135a581fddff64c0572a708fba0ae1b1fc27540b817f |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.6.0
```

## 1.5.0

### Release records

Release date: May 1, 2023

New Feature

* Add plug-in Extension mechanism [#648](https://github.com/alibaba/ilogtail/pull/648)
* flusher\_HTTP plug-in supports custom authentication, filtering, and fusing through Extension mechanism [#648](https://github.com/alibaba/ilogtail/pull/648)
* Add Fusher plug-in: flusher\_Loki [#259](https://github.com/alibaba/ilogtail/issues/259)
* 新增Processor插件: processor\_string\_replace [#757](https://github.com/alibaba/ilogtail/pull/757)
* Supports automatically adding tags [#812](https://github.com/alibaba/ilogtail/issues/812) to data through files in C ++ acceleration mode.
* Add controlled switching strategy for SLS Data Server Endpoint [#802](https://github.com/alibaba/ilogtail/issues/802)
* Support iLogtail to control and send data through network proxy [#806](https://github.com/alibaba/ilogtail/issues/806)
* Add Windows compilation scripts support compiling x86 and x86-64 versions [#327](https://github.com/alibaba/ilogtail/issues/327)
* Profiling function supports goprofile pull mode [#730](https://github.com/alibaba/ilogtail/issues/730)
* V2 pipeline fully supports OTLP Log model [#779](https://github.com/alibaba/ilogtail/issues/779)

Optimization

* enable\_env\_ref\_in\_config is enabled by default to support the replacement of environment variables in the configuration [#744](https://github.com/alibaba/ilogtail/issues/744).
* processor\_split\_key\_value plug-in performance optimization and add multi-character reference [#762](https://github.com/alibaba/ilogtail/issues/762)

Troubleshooting

* Fix the problem that some Tag fields in the flusher option cannot be renamed when using the flusher\_Kafka \_v2/TagFieldsRename \_pulsar plug-in [#744](https:// github.com/alibaba/ilogtail/issues/744)
* Fix the problem that indexes cannot be automatically created when creating logstore in query mode [#798](https://github.com/alibaba/ilogtail/issues/798)
* Fix the problem of Filtering logs even if multiple keys are matched in processor\_filter\_key [#816](https://github.com/alibaba/ilogtail/issues/816)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.5.0.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.5.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.5.0/ilogtail-1.5.0.Linux-amd64.tar.gz) | Linux | x86-64 | ccb7e637bc7edc4e9fe22ab3ac79cedee63d80678711e0efc77122387ec73882 |
| [ilogtail-1.5.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.5.0/ilogtail-1.5.0.Linux-arm64.tar.gz) | Linux | arm64 | f1d7940e08ee51f2c66d963d91c16903658ab1e0fc856002351248c94d0e6b0a |
| [ilogtail-1.5.0.windows-amd64.zip](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.5.0/ilogtail-1.5.0.windows-amd64.zip)   | Windows | x86-64 | 7b3ccbfcca18e1ffeb2e5db06b32fbcac3e8b7ecf8113a7006891bbe3c1a5a84 |

### Docker image

**Docker Pull**&#x20;

``` bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.5.0
```

## 1.4.0

### Release records

Release date: March 21, 2023

New Feature

* Add Pulsar output plug-in flusher\_Pulsar [#540](https://github.com/alibaba/ilogtail/issues/540)
* Added ClickHouse output plug-in flusher\_ClickHouse [#112](https://github.com/alibaba/ilogtail/issues/112)
* 新增Elasticsearch输出插件flusher\_elasticsearch [#202](https://github.com/alibaba/ilogtail/issues/202)
* Add Open Telemetry input plug-in service\_otlp [#637](https://github.com/alibaba/ilogtail/issues/637)
* Use plugins.yml cropping plug-in and external\_plugins.yml to introduce external plug-in library [#625](https://github.com/alibaba/ilogtail/issues/625)
* Add the metadata processing plug-in processor\_cloud\_meta [#692](https://github.com/alibaba/ilogtail/pull/692)
* Update Open Telemetry output plug-in added Metric/Trace support, and rename it flusher\_otlp [#646](https://github.com/alibaba/ilogtail/pull/646)
* Update HTTP input service new Pyroscope protocol support [#653](https://github.com/alibaba/ilogtail/issues/653)

Optimization

* Update Kafka V2 flusher support TLS and Kerberos authentication [#601](https://github.com/alibaba/ilogtail/issues/601)
* Update HTTP output support adding dynamic Header [#643](https://github.com/alibaba/ilogtail/pull/643)
* Update support for new SLS Config parameters such as cold storage through ENV configuration Logstore [#687](https://github.com/alibaba/ilogtail/issues/687)

Troubleshooting

* Fix time zone-related problems. Ignore the time zone adjustment option when the system time and log parsing fail [#550](https://github.com/alibaba/ilogtail/issues/550)
* Fix the problem of repeated log collection or file name disorder caused by inode reuse [#597](https://github.com/alibaba/ilogtail/issues/597)
* Fix the problem that the input plug-in of Prometheus automatically switches to streaming mode and gets stuck [#684](https://github.com/alibaba/ilogtail/pull/684)
* Fix the problem that Grok plug-in may get stuck when parsing Chinese [#644](https://github.com/alibaba/ilogtail/issues/644)
* Fix the problem that containers introduced in version 1.2.1 find that the memory usage is too high [#661](https://github.com/alibaba/ilogtail/issues/661)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.4.0.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.4.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.4.0/ilogtail-1.4.0.Linux-amd64.tar.gz) | Linux | x86-64 | d48fc6e8c76f117651487a33648ab6de0e2d8dd24ae399d9a7f534b81d639a61 |
| [ilogtail-1.4.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.4.0/ilogtail-1.4.0.Linux-arm64.tar.gz) | Linux | arm64 | 1d488d0905e0fb89678e256c980e491e9c1c0d3ef579ecbbc18360afdcc1a853 |

### Docker image

**Docker Pull**&#x20;

```bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.4.0
```

## 1.3.1

### Release records

Release date: December 13, 2022

New Feature

Optimization

Troubleshooting

* Problems that may fail to obtain IP addresses and cause crashes [#576](https://github.com/alibaba/ilogtail/issues/576)
* The problem that the ARM version cannot run on Ubuntu 20.04 [#570](https://github.com/alibaba/ilogtail/issues/570)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.3.1.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.3.1.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.3.1/ilogtail-1.3.1.Linux-amd64.tar.gz) | Linux | x86-64 | d74e2e8683fa9c01fcaa155e65953936c18611bcd068bcbeced84a1d7f170bd1 |
| [ilogtail-1.3.1.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.3.1/ilogtail-1.3.1.Linux-arm64.tar.gz) | Linux | arm64 | d9a72b2ed836438b9d103d939d11cf1b0a73814e6bb3d0349dc0b6728b981eaf |

### Docker image

**Docker Pull**&#x20;

```bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.3.1
```

## 1.3.0

### Release records

Release date: November 30, 2022

New Feature

* Add the new version of Kafka output plug-in, support custom output format [#218](https://github.com/alibaba/ilogtail/issues/218)
* Support Open Telemetry protocol log output through grpc [#418](https://github.com/alibaba/ilogtail/pull/418)
* Supports accessing Open Telemetry protocol logs through HTTP [#421](https://github.com/alibaba/ilogtail/issues/421)
* Add desensitization plug-in processor\_desensitize [#525](https://github.com/alibaba/ilogtail/pull/525)
* Support zstd Compression coding [#526](https://github.com/alibaba/ilogtail/issues/526) when writing to SLS

Optimization

* Reduce the size of the published binary package [#433](https://github.com/alibaba/ilogtail/pull/433)
* Use HTTPS protocol [#505](https://github.com/alibaba/ilogtail/issues/505) when creating SLS resources by using ENV.
* Select SLS Logstore  When creating query mode [#502](https://github.com/alibaba/ilogtail/issues/502)
* When using the plug-in to process, the offset of output content in the file is also supported [#395](https://github.com/alibaba/ilogtail/issues/395)
* By default, it supports continuous context [#522](https://github.com/alibaba/ilogtail/pull/522) when collecting container standard output.
* Prometheus data access memory optimization [#524](https://github.com/alibaba/ilogtail/pull/524)

Troubleshooting

* Fix potential FD leakage and event omission problems in Docker environment [#529](https://github.com/alibaba/ilogtail/issues/529)
* Fix the problem of file handle leakage during configuration update [#420](https://github.com/alibaba/ilogtail/issues/420)
* IP Address Resolution error under special host name [#517](https://github.com/alibaba/ilogtail/pull/517)
* Fix the problem of repeated file collection when multiple configuration paths have parent-child directory relationship [#533](https://github.com/alibaba/ilogtail/issues/533)

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.3.0.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.3.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.3.0/ilogtail-1.3.0.Linux-amd64.tar.gz) | Linux | x86-64 | 5ef5672a226089aa98dfb71dc48b01254b9b77c714466ecc1b6c4d9c0df60a50 |
| [ilogtail-1.3.0.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.3.0/ilogtail-1.3.0.Linux-arm64.tar.gz) | Linux | arm64 | 73d33d4cac90543ea5c2481928e090955643c6dc6839535f53bfa54b6101704d |

### Docker image

**Docker Pull**&#x20;

```bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.3.0
```

## 1.2.1

### Release records

Release date: September 28, 2022

New Feature

* Add the PostgreSQL collection plug-in.
* Add the SqlServer collection plug-in.
* Add the Grok processing plug-in.
* Supports collecting JMX performance metrics.
* Added the log context aggregation plug-in to support context browsing and log topic extraction after the plug-in processes.

Optimization

* Supports the standard output collection of the second-drop container.
* Shorten the release time of the exited container handle.

Troubleshooting

* Fine timestamp resolution in Apsara log format.

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.2.1.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.2.1.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.2.1/ilogtail-1.2.1.Linux-amd64.tar.gz) | Linux | x86-64 | 8a5925f1bc265fd5f55614fcb7cebd571507c2c640814c308f62c498b021fe8f |
| [ilogtail-1.2.1.linux-arm64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.2.1/ilogtail-1.2.1.Linux-arm64.tar.gz) | Linux | arm64 | 04be03f03eb722a3c9ba1b45b32c9be8c92cecabbe32bdc4672d229189e80c2f |

### Docker image

**Docker Pull**&#x20;

```bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.2.1
```

## 1.1.1

### Release records

Release date: August 22, 2022

New Feature

* Add the Logtail CSV processing plug-in.
* Supports Layer -4 and layer -7 network traffic analysis through eBPF, and supports HTTP, MySQL, PgSQL, Redis, and DNS protocols.

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.1.1.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.1.1.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.1.1/ilogtail-1.1.1.Linux-amd64.tar.gz) | Linux | x86-64 | 2330692006d151f4b83e4b8be0bfa6b68dc8d9a574c276c1beb6637c4a2939ec |

### Docker image

**Docker Pull**&#x20;

```bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.1
```

## 1.1.0

### Release records

Release date: June 29, 2022

New Feature

* Open source C ++ core module code
* netping plug-in supports httping and DNS resolution time.

[Details and source code](https://github.com/alibaba/ilogtail/blob/main/changes/v1.1.0.md)

### Download

| File name | System | Architecture | SHA256 check code |
| -------------------------------------------------------------------------------------------------------------------------------------------- | ----- | ------ | ---------------------------------------------------------------- |
| [ilogtail-1.1.0.linux-amd64.tar.gz](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/1.1.0/ilogtail-1.1.0.Linux-amd64.tar.gz) | Linux | x86-64 | 2f4eadd92debe17aded998d09b6631db595f5f5aec9c8ed6001270b1932cad7d |

### Docker image

**Docker Pull**&#x20;

{% hint style="warning" %}
The image with tag 1.1.0 lacks the necessary environment variables and cannot support file collection and checkpoint in the container. Use the 1.1.0-k8s-patch or pull the latest
{% endhint %}

```bash
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.0-k8s-patch
```
