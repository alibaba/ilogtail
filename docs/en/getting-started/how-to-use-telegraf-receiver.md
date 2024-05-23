# How do I collect Telegraf data?

## Preface

Alibaba has officially opened the observable data collector iLogtail. As an internal observable data collection infrastructure of Alibaba, iLogtail collects various observable data such as logs, monitoring, Trace, and events from Alibaba Group and Ant Financial. This topic describes how iLogtail work with Telegraf,Collect metric data.

## Collection configuration

iLogtail the current collection configuration is fully compatible with Telegraf [configuration file](https://github.com/influxdata/telegraf/blob/master/docs/CONFIGURATION.md), iLogtail will start two plug-ins to work together, namely service_telegraf plug-in and service_http_server plug-in. service_telegraf is responsible for establishing the association between iLogtail and Telegraf processes,The configuration management of the Telegraf Agent is controlled by the iLogtail, while service_http_server is responsible for iLogtail the specific data transmission between the plug-in and the Telegraf Agent.

1\. Configure the parameters of the service_telegraf control plug-in:

| Parameter | Description | Default value |
| --- | --- | --- |
| Detail | Telegraf collection configuration in yaml format | |

2\. Configure the parameters of the service_http_server collection plug-in:

| Parameter | Description | Default value |
| --- | --- | --- |
| Format | Data Format. For Telegraf Agent, set the parameter. | |
| Address | Listener Address | |
| ReadTimeoutSec | Read timeout | 10 |
| ShutdownTimeoutSec | Shutdown timeout | 5 |
| MaxBodySize | Maximum body size | 64*1024*1024 |
| UnlinkUnixSock | Whether to forcibly release the listener if the listener address is unix socket before startup | true |

The following is a simple collection configuration:

```json
[
    {
        "detail":{
            "Detail":"\n# Read Redis's basic status information\n[[inputs.redis]]\n  interval = \"30s\"\n  ## specify servers via a url matching:\n  ##  [protocol://][:password]@address[:port]\n  ##  e.g.\n  ##    tcp://localhost:6379\n  ##    tcp://:password@192.168.99.100\n  ##\n  ## If no servers are specified, then localhost is used as the host.\n  ## If no port is specified, 6379 is used\n  ## servers = [\"tcp://host:port\"]\n  servers = [\"tcp://127.0.0.1:6379\"]\n  ## specify server password\n  # password = \"s#cr@t%\"\n  # password = \"\"\n\n  ## Optional TLS Config\n  # tls_ca = \"/etc/telegraf/ca.pem\"\n  # tls_cert = \"/etc/telegraf/cert.pem\"\n  # tls_key = \"/etc/telegraf/key.pem\"\n  ## Use TLS but skip chain & host verification\n  # insecure_skip_verify = true\n  [inputs.redis.tags]\n    __metricstore__ = \"ljp-test-bj_telegraf_test\"\n    cluster = \"test\"\n  \n    key = \"value\"\n  \n\n\n[[outputs.influxdb]]\n  ## The full HTTP or UDP URL for your InfluxDB instance.\n  ##\n  ## Multiple URLs can be specified for a single cluster, only ONE of the\n  ## urls will be written to each interval.\n  # urls = [\"unix:///var/run/influxdb.sock\"]\n  # urls = [\"udp://127.0.0.1:8089\"]\n  urls = [\"unix:///var/run/ljp-test-bj_telegraf_test.sock\"]\n  tagexclude = [\"__metricstore__\"]\n  skip_database_creation = true\n  [outputs.influxdb.tagpass]\n    __metricstore__ = [\"ljp-test-bj_telegraf_test\"]\n"
        },
        "type":"service_telegraf"
    },
    {
        "detail":{
            "Address":"unix:///var/run/ljp-test-bj_telegraf_test.sock",
            "Format":"influx"
        },
        "type":"service_http_server"
    }
]
```

## Data collection format

iLogtail Telegraf data and logs collected by Metrics also follow the [iLogtail transport layer protocol](https://github.com/alibaba/ilogtail/blob/main/pkg/protocol/proto/sls_logs.proto), the current data transmission field is in the following format, which is the same as the Prometheus collection format.

| Transfer field | Meaning |
| --- | --- |
| \_\_name__ | Metric name, which is the same as the Prometheus exporter metric name. |
| \_\_labels__ | 指标Label 熟悉，数据格式为`{label_a_nane}#$#{label_a_value}&#124;{label_b_nane}#$#{label_b_value}`，例如`instance#$#exporter:18080&#124;job#$#prometheus` |
| \_\_time_nano__ | Collection time |
| \_\_value__ | Metric value |

### Conversion rules

The data rules collected by Telegraf Agent are in the format of {type},{tags} {key & value pairs} {time}.

```bash
mysql,host=Vm-Req-170328120400894271-tianchi113855.tc,server=rm-bp1eomqfte2vj91tkjo.mysql.rds.aliyuncs.com:3306 bytes_sent=19815071437i,com_assign_to_keycache=0i,com_alter_event=0i,com_alter_function=0i,com_alter_server=0i,com_alter_table=0i,aborted_clients=7738i,binlog_cache_use=136756i,binlog_stmt_cache_use=136759i,com_alter_procedure=0i,binlog_stmt_cache_disk_use=0i,bytes_received=6811387420i,com_alter_db_upgrade=0i,com_alter_instance=0i,aborted_connects=7139i,binlog_cache_disk_use=0i,com_admin_commands=3478164i,com_alter_db=0i,com_alter_tablespace=0i,com_alter_user=0i 1595818360000000000
```

The iLogtail converts the received Telegraf data into Prometheus class format. The conversion rule is as follows:

- \_\_name__：{type}:{key}
- \_\_labels__： {tags}
- \_\_time_nano__：{time}
- \_\_value__：{value}

## Local collection Telegraf collection of Redis metric data

1\. Prepare the Linux environment.

2\. [Download](https://github.com/alibaba/ilogtail/releases) install the latest ilogtail version.

```shell
# Decompress the tar package
$ tar zxvf logtail-linux64.tar.gz

# View the directory structure
$ ll logtail-linux64
drwxr-xr-x   3 500 500  4096 bin
drwxr-xr-x 184 500 500 12288 conf
-rw-r--r--   1 500 500   597 README
drwxr-xr-x   2 500 500  4096 resources

# Enter the bin directory
$ cd logtail-linux64/bin
$ ll
-rwxr-xr-x 1 500 500 10052072 ilogtail_1.0.28 # ilogtail executable file
-rwxr-xr-x 1 500 500     4191 ilogtaild  
-rwxr-xr-x 1 500 500     5976 libPluginAdapter.so
-rw-r--r-- 1 500 500 89560656 libPluginBase.so
-rwxr-xr-x 1 500 500  2333024 LogtailInsight
```

3\. Create a collection configuration directory.

```bash
# 1. create sys_conf_dir
$ mkdir sys_conf_dir

#2. Create ilogtail_config.json and complete the configuration.
##### logtail_sys_conf_dir：$pwd/sys_conf_dir/
##### config_server_address is fixed and remains unchanged.
$ pwd
/root/bin/logtail-linux64/bin
$ cat ilogtail_config.json
{
     "logtail_sys_conf_dir": "/root/bin/logtail-linux64/bin/sys_conf_dir/",  

     "config_server_address" : "http://logtail.cn-zhangjiakou.log.aliyuncs.com"
}

#3. The directory structure at this time
$ ll
-rwxr-xr-x 1  500  500 ilogtail_1.0.28
-rw-r--r-- 1 root root ilogtail_config.json
-rwxr-xr-x 1  500  500 ilogtaild
-rwxr-xr-x 1  500  500 libPluginAdapter.so
-rw-r--r-- 1  500  500 libPluginBase.so
-rwxr-xr-x 1  500  500 LogtailInsight
drwxr-xr-x 2 root root sys_conf_dir
```

4\. Set the collection configuration file, and write the following content into the sys_conf_dir/user_local_config.json file. The following core configuration is the plugin part, and the core part is the plugin part. This part mainly describes the collection configuration Telegraf,Write the collected Telegraf data to the local telegraf.log.

```json
{
    "metrics":{
        "test-telegraf":{
            "aliuid":"",
            "category":"",
            "create_time":1640692891,
            "defaultEndpoint":"",
            "delay_alarm_bytes":0,
            "enable":true,
            "enable_tag":false,
            "filter_keys":[

            ],
            "filter_regs":[

            ],
            "group_topic":"",
            "local_storage":true,
            "log_type":"plugin",
            "log_tz":"",
            "max_send_rate":-1,
            "merge_type":"topic",
            "plugin":{
                "inputs":[
                    {
                        "detail":{
                            "Detail":"\n# Read Redis's basic status information\n[[inputs.redis]]\n  interval = \"30s\"\n  ## specify servers via a url matching:\n  ##  [protocol://][:password]@address[:port]\n  ##  e.g.\n  ##    tcp://localhost:6379\n  ##    tcp://:password@192.168.99.100\n  ##\n  ## If no servers are specified, then localhost is used as the host.\n  ## If no port is specified, 6379 is used\n  ## servers = [\"tcp://host:port\"]\n  servers = [\"tcp://127.0.0.1:6379\"]\n  ## specify server password\n  # password = \"s#cr@t%\"\n  # password = \"\"\n\n  ## Optional TLS Config\n  # tls_ca = \"/etc/telegraf/ca.pem\"\n  # tls_cert = \"/etc/telegraf/cert.pem\"\n  # tls_key = \"/etc/telegraf/key.pem\"\n  ## Use TLS but skip chain & host verification\n  # insecure_skip_verify = true\n  [inputs.redis.tags]\n    __metricstore__ = \"ljp-test-bj_telegraf_test\"\n    cluster = \"test\"\n  \n    key = \"value\"\n  \n\n\n[[outputs.influxdb]]\n  ## The full HTTP or UDP URL for your InfluxDB instance.\n  ##\n  ## Multiple URLs can be specified for a single cluster, only ONE of the\n  ## urls will be written to each interval.\n  # urls = [\"unix:///var/run/influxdb.sock\"]\n  # urls = [\"udp://127.0.0.1:8089\"]\n  urls = [\"unix:///var/run/ljp-test-bj_telegraf_test.sock\"]\n  tagexclude = [\"__metricstore__\"]\n  skip_database_creation = true\n  [outputs.influxdb.tagpass]\n    __metricstore__ = [\"ljp-test-bj_telegraf_test\"]\n"
                        },
                        "type":"service_telegraf"
                    },
                    {
                        "detail":{
                            "Address":"unix:///var/run/ljp-test-bj_telegraf_test.sock",
                            "Format":"influx"
                        },
                        "type":"service_http_server"
                    }
                ],
                "flushers":[
                    {
                        "detail":{
                            "FileName":"./telegraf.log"
                        },
                        "type":"flusher_stdout"
                    }
                ]
            },
            "priority":0,
            "project_name":"",
            "raw_log":false,
            "region":"",
            "send_rate_expire":0,
            "sensitive_keys":[

            ],
            "shard_hash_key":[

            ],
            "tz_adjust":false,
            "version":1
        }
    }
}
```

5\. Set Telegraf

```bash
cd sys_conf_dir
# Download Telegraf
wget https://logtail-release-cn-hangzhou.oss-cn-hangzhou.aliyuncs.com/linux64/telegraf/1.0.29/telegraf.tar.gz

$tar -xvf telegraf.tar.gz
telegraf/
telegraf/version
telegraf/javaagent/
telegraf/javaagent/jolokia-jvm.jar
telegraf/telegrafd
telegraf/arch_type
telegraf/conf.d/
telegraf/telegraf

chmod 755 telegraf/telegrafd
```

6\. Start the redis container and expose Port 6379.

```bash
docker pull redis
docker run -p 6379:6379 -d redis:latest redis-server
```

7\. logtail startup, you can see Telegraf data and save it in the telegraf.log file.

```bash
sudo ./ilogtail_1.0.29 --ilogtail_daemon_flag=false

2022-02-21 23:46:35 {"__name__":"redis:instantaneous_output_kbps","__labels__":"cluster#$#test|host#$#sls-backend-server-test011122076082.na131|key#$#value|port#$#6379|replication_role#$#master|server#$#127.0.0.1","__time_nano__":"1645458390000000000","__value__":"0","__time__":"1645458390"}
2022-02-21 23:46:35 {"__name__":"redis:sync_full","__labels__":"cluster#$#test|host#$#sls-backend-server-test011122076082.na131|key#$#value|port#$#6379|replication_role#$#master|server#$#127.0.0.1","__time_nano__":"1645458390000000000","__value__":"0","__time__":"1645458390"}
2022-02-21 23:46:35 {"__name__":"redis:mem_clients_slaves","__labels__":"cluster#$#test|host#$#sls-backend-server-test011122076082.na131|key#$#value|port#$#6379|replication_role#$#master|server#$#127.0.0.1","__time_nano__":"1645458390000000000","__value__":"0","__time__":"1645458390"}
2022-02-21 23:46:35 {"__name__":"redis:client_recent_max_output_buffer","__labels__":"cluster#$#test|host#$#sls-backend-server-test011122076082.na131|key#$#value|port#$#6379|replication_role#$#master|server#$#127.0.0.1","__time_nano__":"1645458390000000000","__value__":"0","__time__":"1645458390"}
```

## Log service collects Redis metric data

1\. Refer to [server environment log collection to SLS](https://github.com/alibaba/ilogtail/blob/main/docs/zh/usecases/How-to-setup-on-host.md) establish a link between the host iLogtail and Alibaba Cloud log service.

2\. 创建MetricStore

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1645543648105-dd8004c3-f0f2-4f3c-a922-fc7376bfb32c.png#clientId=ud1ed2f8e-0c3c-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=901&id=u00e79867&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1802&originWidth=3582&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1331965&status=done&style=none&taskId=u6bca1828-316a-44bf-8d79-94ec91483a6&title=&width=1791)

3\. On The __create machine group__ tab, create a machine group, and then apply the machine group on the __Machine Group configuration__ tab. Select a machine group and move the machine Group from __source Machine Group__ to __application machine group__.

4\. Create a redis collection configuration and select Redis monitoring.

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1645543646377-23d0cdc0-c42b-4205-b91c-a2f8bdbb6879.png#clientId=ud1ed2f8e-0c3c-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=840&id=u376e9200&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1680&originWidth=3582&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1169149&status=done&style=none&taskId=u1cfd528e-f97a-42b5-b78c-020e8e5d919&title=&width=1791)

5\. Configure the Redis monitoring and collection configuration.

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1645543645507-6d15ae85-bafe-4df0-aa88-d9838f881daf.png#clientId=ud1ed2f8e-0c3c-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=786&id=u59ab1441&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1572&originWidth=2758&originalType=binary&ratio=1&rotation=0&showTitle=false&size=383606&status=done&style=none&taskId=u4f2d28e4-09b0-43e8-aef6-cd942a775f0&title=&width=1379)

6\. After the collection configuration is successful, you can open the visual Kanban to view and display redis data.

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1645543648098-476ea724-8fb0-4e7b-aa06-96ec6182b58c.png#clientId=ud1ed2f8e-0c3c-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=747&id=ud075cad9&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1494&originWidth=2974&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1090286&status=done&style=none&taskId=u3313b0bb-f694-4c77-badb-6998f854f2c&title=&width=1487)

## Summary

iLogtail provides a complete Telegraf data collection capability, which allows you to convert Telegraf collection to iLogtail collection without modifying migration. Based on the MetricStore capabilities of log service, a standard PromQL Metrics query solution is built,You do not need to select multiple Metrics Metrics.

## Reference

- [Telegraf configuration document](https://github.com/influxdata/telegraf/blob/master/docs/CONFIGURATION.md)
- [Access Redis Data](https://help.aliyun.com/document_detail/185092.html)
- [iLogtail transport layer protocol](https://github.com/alibaba/ilogtail/blob/main/pkg/protocol/proto/sls_logs.proto)
- [Server environment log collection to SLS](https://github.com/alibaba/ilogtail/blob/main/docs/zh/usecases/How-to-setup-on-host.md)
