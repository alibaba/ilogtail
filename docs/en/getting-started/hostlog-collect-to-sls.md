# Collect business logs to SLS in the host environment

'iLogtail' is a observable data collection 'Agent' developed by the Alibaba Cloud log service (SLS) team. It has many production-level features such as lightweight, high-performance, and automated configuration. It can be used to collect telemetry data in various environments such as physical machines, virtual machines, and 'Kubernetes.iLogtail has served the observability collection of tens of thousands of customer hosts and containers on Alibaba Cloud. Its core product lines, such as Taobao, Tmall, Alipay, Cainiao, and AMAP, are also default tools for collecting various observable data such as logs, monitoring, and Trace. Currently, iLogtail has tens of millions of installation capacity,Dozens of PB of observable data are collected every day, which are widely used in various scenarios such as online monitoring, problem analysis/positioning, Operation analysis, and security analysis, and have verified its strong performance and stability in actual combat.
In today's cloud-native era, we firmly believe that open source is the best development strategy for iLogtail and the way to release its maximum value. Therefore, we decided to open the 'iLogtail' source, hoping to work with many developers to build iLogtail into a world-class observable data collector.

## Background

As an important part of observability construction, logs can record detailed access requests and error messages, and often play an important role in business analysis and problem location. In general development scenarios, when log analysis is required, grep is often used to search for the corresponding keywords in the log file;However, in a large-scale distributed production environment, this method is inefficient. The common solution is to establish a centralized log collection system to collect, manage, and analyze logs on all nodes. Currently, the mainstream open source solution in the market is to build a log collection and analysis system based on ELK.

![elk-arch](<../.gitbook/assets/getting-started/collect-to-kafka/elk-arch.png>)

In this architecture, 'Filebeat' is deployed on the business cluster to collect raw logs, and the collected logs are sent to the message queue 'Kafka' cluster. After that, 'Logstash' consumes data from 'Kafka', and after filtering and processing,Write standardized logs to the 'Elasticsearch' cluster for storage. Finally, 'Kibana' is presented to the user for query. Although this architecture can provide complete log collection and analysis functions, it involves many components and has high deployment complexity in large-scale production environments,In addition, ES may be unstable under high traffic, resulting in high O & M costs.

The SLS service provided by Alibaba Cloud is a cloud-based hosting service that is exclusively positioned in the log/time series observable data analysis Scenario. Compared with ELK, it has made many Customized developments in the log field, and has good performance in ease of use, functional completeness, performance, and cost. 'iLogtail' is an official standard observable data collector for SLS,In [log collection performance](https://github.com/alibaba/ilogtail/blob/main/docs/zh/performance/Performance-compare-with-filebeat.md) and [K8s](https://developer.aliyun.com/article/806369) support have good experience; 'iLogtail' has obvious performance advantages, which can lead some data processing to reduce storage costs effectively.

![sls-collect-arch](<../.gitbook/assets/getting-started/collect-to-sls/sls-collect-arch.png>)

Currently, **Community Edition** 'iLogtail' also provides good support for SLS. This article will introduce in detail how to use **Community Edition** 'iLogtail' and quickly build a high-availability and high-performance log collection and analysis system in combination with SLS cloud services.

Note: 'iLogtail' **Community Edition** compared with 'iLogtail' Enterprise edition, the core collection capabilities are basically the same, but the control and observability capabilities are weakened. These capabilities can only be developed with the SLS server. Welcome to [iLogtail Enterprise Edition](https://help.aliyun.com/document_detail/95923.html) experience. For more information about the differences between the two versions, see [link](https://ilogtail.gitbook.io/ilogtail-docs/about/compare-editions).

## SLS overview

Log service (SLS) is a cloud-native observation and analysis platform that provides large-scale, low-cost, and real-time platform services for Log, Metric, and Trace data. Log service provides features such as data collection, processing, query and analysis, visualization, alerting, consumption, and delivery, to improve your digitization capabilities in R & D, O & M, operations, and security scenarios.
SLS allows you to quickly build your own observable analysis platform and quickly enjoy various data services provided by SLS, including query and analysis, visualization, and alerts.

- Query and analysis
- Supports exact query, fuzzy query, full-text query, and Field query.
- Use SQL as the query and analysis framework, and integrate PromQL syntax and machine learning functions into the framework.

![sls-search](<../.gitbook/assets/getting-started/collect-to-sls/sls-search.png>)

- Visualization
- Based on the unified query and analysis engine of SLS, the query and analysis results are presented in the form of charts to clearly present the global situation.
- Supports docking with third-party visualization tools.

![sls-visualization](<../.gitbook/assets/getting-started/collect-to-sls/sls-visualization.png>)

- Monitoring and alerting: an intelligent O & M platform that provides one-stop alarm monitoring, noise reduction, transaction management, and notification distribution.

![sls-alert](<../.gitbook/assets/getting-started/collect-to-sls/sls-alert.png>)

## Actual operation

The following describes how to use the 'iLogtail' Community Edition to collect host environment business logs to SLS.

### Scenario

Collect '/ root/bin/input_data/access.log', '/root/bin/input_data/error.log', and write the collected logs to SLS.
'access.log' requires regular parsing; 'error.log' indicates single-line text printing.
If you have used 'iLogtail' to collect logs to 'Kafka', you can keep double write during the migration phase. After the migration is stable, you can delete the 'Kafka fleful' configuration.

![collect-to-sls-and-kafka](<../.gitbook/assets/getting-started/collect-to-sls/collect-to-sls-and-kafka.png>)

### Prerequisites

- Log on to the Alibaba Cloud SLS console and [activate SLS service](https://help.aliyun.com/document_detail/54604.html#section-j4p-xt3-arc).
- A Project has been created. The two logstore are access-log and error-log. For more information, see [Create Project](https://help.aliyun.com/document_detail/48984.htm#section-ahq-ggx-ndb) and [Create Logstore](https://help.aliyun.com/document_detail/48990.htm#section-v52-2jx-ndb)。

![create-logstore](<../.gitbook/assets/getting-started/collect-to-sls/create-logstore.png>)

- Enable [full-text index](https://help.aliyun.com/document_detail/90732.html).
- Go to the home page of the Project and view [access domain name](https://help.aliyun.com/document_detail/29008.html?spm=5176.2020520112.help.dexternal.5d8b34c0QXLYgp)。

![endpoint](<../.gitbook/assets/getting-started/collect-to-sls/endpoint.png>)

### Install iLogtail

- Download

```shell
$ wget https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/latest/ilogtail-latest.linux-amd64.tar.gz
tar -xzvf ilogtail-latest.linux-amd64.tar.gz
$ cd ilogtail-<version>
$ ll
drwxrwxr-x 5 505 505      4096 7月  10 18:00 example_config
-rwxr-xr-x 1 505 505  84242040 7月  11 00:00 ilogtail
-rwxr-xr-x 1 505 505     16400 7月  11 00:00 libPluginAdapter.so
-rw-r--r-- 1 505 505 115963144 7月  11 00:00 libPluginBase.so
-rw-rw-r-- 1 505 505     11356 7月  11 00:00 LICENSE
-rw-rw-r-- 1 505 505      4834 7月  11 00:00 README.md
-rw-rw-r-- 1  505  505    118 7月  14 11:22 ilogtail_config.json
drwxr-xr-x 2 root root    4096 7月  12 09:55 config
```

- Obtain the Alibaba Cloud AK and configure it.

```shell
$ cat ilogtail_config.json
{
   "default_access_key_id": "xxxxxx",
   "default_access_key": "yyyyy"
}
```

- Collection configuration

Create two collection configurations for 'access_log' and 'error_log' in 'config/local'. The two collection configurations collect logs to different 'logstore' and 'Kafka' topic.Dual-write is applicable to scenarios where you migrate data from 'Kafka' to SLS. If the migration is stable, you can delete 'fleful_kafka 'and only keep 'fleful_sls.

```yaml
# Configure access log collection
$ cat config/local/access_log.yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /root/bin/input_data/access.log
processors:
  - Type: processor_regex
    SourceKey: content
Regex: ([\ d \.]+) \ S \ S \ \[(\ S +) \ S \ \] \ "(\ w +) ([^ \"]*)\ "([\ d \.]+) (\ d +) (\ d +) (\ d + |-) \" ([^ \ "]*)\" \ "([^ \"]*)\"
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
flushers:
  - Type: flusher_sls
    Region: cn-hangzhou
Endpoint: cn-hangzhou .log.aliyuncs.com
    Project: test-ilogtail
Logstore: Access-Log
  - Type: flusher_kafka
    Brokers:
-Localhost: 9092
    Topic: access-log
```

```yaml
# Incorrect log collection configuration
$ Cat Config/local/error_log.yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /root/bin/input_data/error.log
flushers:
  - Type: flusher_sls
Endpoint: cn-hangzhou .log.aliyuncs.com
    Project: test-ilogtail
Logstore: Access-Log
  - Type: flusher_kafka
    Brokers:
-Localhost: 9092
    Topic: error-log
```

```shell
$ tree config/local/
config/local/
-access_log.yaml
│ ── error_log.yaml
```

- Start

```shell
nohup ./ilogtail > stdout.log 2> stderr.log &
```

### Verify

- Access log verification to check logstore data is normal.

```shell
# Write access logs
Echo '127.0.0.1 - - [10/Aug/2017:14:57:51 + 0800] "POST /PutData?Category = YunOAccountOpLog HTTP/1.1" 0.024 18204 200 37 "-" "aliyun-sdk-java"' >> /root/bin/input_data/access.log
```

![sls-access-log](<../.gitbook/assets/getting-started/collect-to-sls/sls-access-log.png>)

- Verify the error log to check logstore the data is normal.

```shell
# Write error logs
echo -e '2022-07-12 10:00:00 ERROR This is a error!\n2022-07-12 10:00:00 ERROR This is a new error!' >> /root/bin/input_data/error.log
```

![sls-error-log](<../.gitbook/assets/getting-started/collect-to-sls/sls-error-log.png>)

## Summary

Above, we introduced how to use iLogtail Community Edition to collect logs to SLS. If you want to experience more in-depth integration between Enterprise Edition iLogtail and SLS, you are welcome to use iLogtail Enterprise Edition and build an observable platform with SLS.

## About iLogtail

As an observable data collector provided by Alibaba Cloud SLS, iLogtail can run in various environments such as servers, containers, K8s, and embedded systems. It can collect hundreds of observable data (such as logs, monitoring, Trace, and events) and has tens of millions of installations. Currently, iLogtail has been officially open-source,Welcome to use and participate in co-construction.

- GitHub：[https://github.com/alibaba/ilogtail](https://github.com/alibaba/ilogtail)

- Community Edition document: [https://ilogtail.gitbook.io/ilogtail-docs/about/readme](https://ilogtail.gitbook.io/ilogtail-docs/about/readme)

- Scan the Communication Group

![chatgroup](../.gitbook/assets/chatgroup.png)
