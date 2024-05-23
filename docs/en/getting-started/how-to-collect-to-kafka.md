# How do I collect business logs to Kafka?

'iLogtail' is a observable data collection 'Agent' developed by the Alibaba Cloud log service (SLS) team. It has many production-level features such as lightweight, high-performance, and automated configuration. It can be used to collect telemetry data in various environments such as physical machines, virtual machines, and 'Kubernetes.iLogtail has served the observability collection of tens of thousands of customer hosts and containers on Alibaba Cloud. Its core product lines, such as Taobao, Tmall, Alipay, Cainiao, and AMAP, are also default tools for collecting various observable data such as logs, monitoring, and Trace. Currently, iLogtail has tens of millions of installation capacity,Dozens of PB of observable data are collected every day, which are widely used in various scenarios such as online monitoring, problem analysis/positioning, Operation analysis, and security analysis, and have verified its strong performance and stability in actual combat.

In today's cloud-native era, we firmly believe that open source is the best development strategy for iLogtail and the way to release its maximum value. Therefore, we decided to open the 'iLogtail' source, hoping to work with many developers to build iLogtail into a world-class observable data collector.

## Background <a href = "#lqmpz" id = "lqmpz"></a>

As an important part of observability construction, logs can record detailed access requests and error messages, and often play an important role in business analysis and problem location. In general development scenarios, when log analysis is required, grep is often used to search for the corresponding keywords in the log file;However, in a large-scale distributed production environment, this method is inefficient. The common solution is to establish a centralized log collection system to collect, manage, and analyze logs on all nodes. Currently, the mainstream open source solution in the market is to build a log collection system based on ELK.

![elk-arch](<../.gitbook/assets/getting-started/collect-to-kafka/elk-arch.png>)

In this architecture, 'Filebeat' is deployed on the business cluster to collect raw logs, and the collected logs are sent to the message queue 'Kafka' cluster. After that, 'Logstash' consumes data from 'Kafka', and after filtering and processing,Write standardized logs to the 'Elasticsearch' cluster for storage. Finally, 'Kibana' is presented to the user for query.

The architecture has two key design considerations:

* Import 'Kafka '.
* Facilitate upstream and downstream decoupling. The collection end can focus on log collection, and the data analysis section can focus on consuming data from 'Kafka'. 'Kafka' also supports repeated consumption, which can effectively reduce the impact of repeated log collection on the end.
* When the business volume increases, it is prone to a sudden increase in Log Volume. Writing directly to ES is likely to cause performance degradation or even the risk of ES cluster downtime. The introduction of 'Kafka' can solve the problem of peak traffic. Cutting the peak and filling the valley can ensure the balance of traffic written to the ES cluster.
* 'Kafka', as a message queue, has the ability of persistence. It does not lose data and has a large amount of data loss. It can prevent data loss.
* Use 'Filebeat' as the collection Agent for the log source.
* 'Filebeat' is a lightweight log transmission tool. As a collection Agent, it can effectively remedy the shortcomings of 'Logstash' (for example, relying on 'java' and consuming too much resources when the data volume is large).

Although 'Filebeat' is introduced as the collection 'Agent' of the log source, which can effectively improve the collection efficiency and resource usage on the terminal, 'Filebeat' is still unable to cope with large traffic scenarios. In addition, Filebeat support is not very good in container scenarios.iLogtail, as an observable data collection 'Agent' developed by the Alibaba Cloud log service (SLS) team, is used in [log collection performance](https://github.com/alibaba/ilogtail/blob/main/docs/zh/performance/Performance-compare-with-filebeat.md) and [K8s](https://developer.aliyun.com/article/806369) support have good experience, so we can introduce 'iLogtail' into the architecture of log collection system. Because 'iLogtail' has obvious performance advantages,It also has strong data processing capabilities, so the log processing carried by 'Logstash' can be carried forward to 'iLogtail', which can effectively reduce the storage costs of users.

In addition, if 'iLogtail' has a natural integration advantage with Alibaba Cloud log service (SLS), when using SLS as a backend storage system, it can be written directly without the need to introduce additional message queues.

![ilogtail-arch](<../.gitbook/assets/getting-started/collect-to-kafka/ilogtail-arch.png>)

## Actual operation

This topic describes how to use the 'iLogtail' Community Edition to collect logs to 'Kafka' to help users build a log collection system.

### Scenario <a href = "#qarop" id = "qarop"></a>

Collect '/ root/bin/input_data/access.log', '/root/bin/input_data/error.log', and write the collected logs to the locally deployed kafka. To do this, we will configure two collection configuration items.
'access.log' requires regular parsing; 'error.log' indicates single-line text printing.

![collection-config](<../.gitbook/assets/getting-started/collect-to-kafka/collection-config.png>)

### Prerequisites <a href = "#hvouy" id = "hvouy"></a>

* Install

```bash
# Download and decompress
$ wget  https://dlcdn.apache.org/kafka/3.2.0/kafka_2.13-3.2.0.tgz
$ tar -xzf kafka_2.13-3.2.0.tgz
$ cd kafka_2.13-3.2.0

# Start
# Start the ZooKeeper service
# Note: Soon, ZooKeeper will no longer be required by Apache Kafka.
$ bin/zookeeper-server-start.sh config/zookeeper.properties &

# Start the Kafka broker service
$ bin/kafka-server-start.sh config/server.properties &
```

* Create two topics to store access-log and error-log.

```bash
bin/kafka-topics.sh --create --topic access-log --bootstrap-server localhost:9092
bin/kafka-topics.sh --create --topic error-log --bootstrap-server localhost:9092
```

* For more deployment instructions, see [link](https://kafka.apache.org/quickstart).

### Install ilogtail <a href = "#gee97" id = "gee97"></a>

* Download

```bash
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
drwxr-xr-x 2 root root    4096 7月  12 09:55 config
```

* Collection configuration

Create two collection configurations for 'access_log' and 'error_log' in 'config/local'. The two collection configurations collect logs to different 'Topic' of 'Kafka.

```yaml
# Configure access log collection
$ cat config/lcoal/access_log.yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /root/bin/input_data/access.log
processors:
  - Type: processor_regex
    SourceKey: content
    Regex: ([\d\.]+) \S+ \S+ \[(\S+) \S+\] \"(\w+) ([^\\"]*)\" ([\d\.]+) (\d+) (\d+) (\d+|-) \"([^\\"]*)\" \"([^\\"]*)\"
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
  - Type: flusher_kafka
    Brokers:
      - localhost:9092
    Topic: access-log
```

```yaml
# Incorrect log collection configuration
$ cat config/local/error_log.yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /root/bin/input_data/error.log
flushers:
  - Type: flusher_kafka
    Brokers:
      - localhost:9092
    Topic: error-log
```

```bash
$ tree config/local/
config/local/
├── access_log.yaml
└── error_log.yaml
```

* Start

```bash
nohup ./ilogtail > stdout.log 2> stderr.log &
```

### Verify <a href = "#ovijv" id = "ovijv"></a>

* Access log verification

```json
# Terminal 1: Start the kafka-console-consumer and consume access-log
$ bin/kafka-console-consumer.sh --topic access-log --from-beginning --bootstrap-server localhost:9092

# Terminal 2: write access logs
$ echo '127.0.0.1 - - [10/Aug/2017:14:57:51 +0800] "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1" 0.024 18204 200 37 "-" "aliyun-sdk-java"' >> /root/bin/input_data/access.log

# Terminal 1: The Written access log is consumed, indicating that the process is normal.
{"Time":1657592120,"Contents":[{"Key":"__tag__:__path__","Value":"/root/bin/input_data/access.log"},{"Key":"ip","Value":"127.0.0.1"},{"Key":"time","Value":"10/Aug/2017:14:57:51"},{"Key":"method","Value":"POST"},{"Key":"url","Value":"/PutData?Category=YunOsAccountOpLog HTTP/1.1"},{"Key":"request_time","Value":"0.024"},{"Key":"request_length","Value":"18204"},{"Key":"status","Value":"200"},{"Key":"length","Value":"37"},{"Key":"ref_url","Value":"-"},{"Key":"browser","Value":"aliyun-sdk-java"}]}
```

* Error log verification

```json
# Terminal 1: Start the kafka-console-consumer and consume error-log
$ bin/kafka-console-consumer.sh --topic error-log --from-beginning --bootstrap-server localhost:9092

# Terminal 2: write error logs
$ echo -e '2022-07-12 10:00:00 ERROR This is a error!\n2022-07-12 10:00:00 ERROR This is a new error!' >> /root/bin/input_data/error.log

# Terminal 1: consumes the written error log, indicating that the process is normal.
{"Time":1657591799,"Contents":[{"Key":"__tag__:__path__","Value":"/root/bin/input_data/error.log"},{"Key":"content","Value":"2022-07-12 10:00:00 ERROR This is a error!"}]}
{"Time":1657591799,"Contents":[{"Key":"__tag__:__path__","Value":"/root/bin/input_data/error.log"},{"Key":"content","Value":"2022-07-12 10:00:00 ERROR This is a new error!"}]}
```

## Summary <a href = "#ovoc3" id = "ovoc3"></a>

Above, we introduced how to use iLogtail Community Edition to collect logs to Kafka. You can work with other open-source software Kafka and ELK to build your own open-source log collection system. Similarly, if you have higher requirements for the stability of collection and query experience,You can also build a commercial observable platform based on SLS.

## About iLogtail

As an observable data collector provided by Alibaba Cloud SLS, iLogtail can run in various environments such as servers, containers, K8s, and embedded systems. It can collect hundreds of observable data (such as logs, monitoring, Trace, and events) and has tens of millions of installations. Currently, iLogtail has been officially open-source,Welcome to use and participate in co-construction.

* GitHub：[https://github.com/alibaba/ilogtail](https://github.com/alibaba/ilogtail)

* Community edition document: [https://ilogtail.gitbook.io/ilogtail-docs/about/readme](https://ilogtail.gitbook.io/ilogtail-docs/about/readme)

* Scan the Communication Group

![chatgroup](../.gitbook/assets/chatgroup.png)
