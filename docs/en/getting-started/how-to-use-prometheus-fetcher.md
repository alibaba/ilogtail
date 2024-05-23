# How do I collect Prometheus Exporter data?

## Preface

Alibaba has officially opened the observable data collector iLogtail. As an internal observable data collection infrastructure of Alibaba, iLogtail collects various observable data such as logs, monitoring, Trace, and events from Alibaba Group and Ant Financial. This topic describes how iLogtail collect Prometheus exporter data.

## Collection configuration

The collection configuration of the iLogtail is fully compatible with the Prometheus [configuration file](https://prometheus.io/docs/prometheus/latest/configuration/configuration/)(the following introduction is 1.0.30 +).

| Parameter | Description | Default value |
| --- | --- | --- |
| Yaml | Collection configuration in yaml format | |
| ConfigFilePath | The path of the collection configuration file. This field is ignored when the Yaml parameter takes effect. | |
| AuthorizationPath | The authentication path, such as the authentication file search path when authorization parameter in the consul_sd_config configuration takes effect. | When the Yaml parameter takes effect, the default path is the program running directory. When the ConfigFilePath parameter takes effect, the default path is the path where the configuration file is located. |

The following is a simple prometheus collection configuration.

```json
{
    "inputs":[
        {
            "detail":{
                "Yaml":"global:
  scrape_interval: 15s 
  evaluation_interval: 15s 
scrape_configs:
  - job_name: "prometheus"
    static_configs:
      - targets: ["exporter:18080"]"
            },
            "type":"service_prometheus"
        }
    ]
}
```

## Data collection format

iLogtail Prometheus data and logs collected by Metrics also follow the [iLogtail transport layer protocol](https://github.com/alibaba/ilogtail/blob/main/pkg/protocol/proto/sls_logs.proto), the current data transmission field is in the following format.

| Transfer field | Meaning |
| --- | --- |
| \_\_name__ | Metric name, which is the same as the Prometheus exporter metric name. |
| \_\_labels__ | Metric Label，format is \{label_a_nane\}\#\$\#\{label_a_value}\|\{label_b_nane}\#\$\#\{label_b_value}，such as instance\#\$\#exporter:18080\|job\#\$\#prometheus |
| \_\_time_nano__ | Collection time |
| \_\_value__ | Metric value |

## E2E quick start

Currently, iLogtail has integrated the E2E test of prometheus, which can be quickly verified in the root path of the iLogtail.
Test Command: TEST_SCOPE = input_prometheus TEST_DEBUG = true make e2e (enable the DEBUG option to view the transmission data details)_

```bash
TEST_DEBUG=true TEST_PROFILE=false  ./scripts/e2e.sh behavior input_prometheus
=========================================
input_prometheus testing case
=========================================
load log config /home/liujiapeng.ljp/data/ilogtail/behavior-test/plugin_logger.xml
2022-01-20 10:46:46 [INF] [load.go:75] [load] load config from: /home/liujiapeng.ljp/data/ilogtail/test/case/behavior/input_prometheus/ilogtail-e2e.yaml
2022-01-20 10:46:46 [INF] [controller.go:129] [WithCancelChain] httpcase controller is initializing....:
2022-01-20 10:46:46 [INF] [controller.go:129] [WithCancelChain] ilogtail controller is initializing....:
2022-01-20 10:46:46 [INF] [validator_control.go:53] [Init] validator controller is initializing....:
2022-01-20 10:46:46 [DBG] [validator_control.go:57] [Init] stage:add rule:fields-check
2022-01-20 10:46:46 [DBG] [validator_control.go:65] [Init] stage:add rule:counter-check
2022-01-20 10:46:46 [INF] [controller.go:129] [WithCancelChain] subscriber controller is initializing....:
2022-01-20 10:46:46 [INF] [controller.go:129] [WithCancelChain] boot controller is initializing....:
2022-01-20 10:46:46 [INF] [boot_control.go:37] [Start] boot controller is starting....:
Creating network "ilogtail-e2e_default" with the default driver
Building exporter
Step 1/8 : FROM golang:1.16
 ---> 71f1b47263fc
Step 2/8 : WORKDIR /src
 ---> Using cache
 ---> d76e92450cbb
Step 3/8 : COPY exporter/* ./
 ---> Using cache
 ---> 55c76b7af4a1
Step 4/8 : RUN go env -w GO111MODULE=on
 ---> Using cache
 ---> 0bbd054e5ca3
Step 5/8 : RUN go env -w GOPROXY=https://goproxy.cn,direct
 ---> Using cache
 ---> 907a360df1d5
Step 6/8 : RUN go build
 ---> Using cache
 ---> 6ef458eccfc2
Step 7/8 : EXPOSE 18080
 ---> Using cache
 ---> ab9ad470b110
Step 8/8 : CMD ["/src/exporter"]
 ---> Using cache
 ---> 6eafc56b0059

Successfully built 6eafc56b0059
Successfully tagged ilogtail-e2e_exporter:latest
Creating ilogtail-e2e_goc_1      ... done
Creating ilogtail-e2e_exporter_1 ... done
Creating ilogtail-e2e_ilogtail_1 ... done
2022-01-20 10:46:50 [INF] [subscriber_control.go:51] [Start] subscriber controller is starting....:
2022-01-20 10:46:50 [INF] [validator_control.go:81] [Start] validator controller is starting....:
2022-01-20 10:46:50 [INF] [logtailplugin_control.go:63] [Start] ilogtail controller is starting....:
2022-01-20 10:46:50 [INF] [logtailplugin_control.go:70] [Start] the 1 times load config operation is starting ...
2022-01-20 10:46:50 [INF] [grpc.go:69] [func1] the grpc server would start in 0s
2022-01-20 10:46:50 [INF] [httpcase_control.go:67] [Start] httpcase controller is starting....:
2022-01-20 10:46:50 [INF] [controller.go:129] [WithCancelChain] testing has started and will last 15s
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"promhttp_metric_handler_requests_in_flight" > Contents:<Key:"__labels__" Value:"instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"1" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"promhttp_metric_handler_requests_total" > Contents:<Key:"__labels__" Value:"code#$#200|instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"0" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"promhttp_metric_handler_requests_total" > Contents:<Key:"__labels__" Value:"code#$#500|instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"0" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"promhttp_metric_handler_requests_total" > Contents:<Key:"__labels__" Value:"code#$#503|instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"0" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"test_counter" > Contents:<Key:"__labels__" Value:"instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"0" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"up" > Contents:<Key:"__labels__" Value:"instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"1" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"scrape_duration_seconds" > Contents:<Key:"__labels__" Value:"instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"0.002" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"scrape_samples_scraped" > Contents:<Key:"__labels__" Value:"instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"5" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"scrape_samples_post_metric_relabeling" > Contents:<Key:"__labels__" Value:"instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"5" >
2022-01-20 10:46:54 [DBG] [validator_control.go:107] [func2] Time:1642646811 Contents:<Key:"__name__" Value:"scrape_series_added" > Contents:<Key:"__labels__" Value:"instance#$#exporter:18080|job#$#prometheus" > Contents:<Key:"__time_nano__" Value:"1642646811274" > Contents:<Key:"__value__" Value:"5" >
2022-01-20 10:47:05 [INF] [httpcase_control.go:107] [func2] httpcase controller is closing....:
2022-01-20 10:47:05 [INF] [httpcase_control.go:108] [func2] httpcase controller is cleaning....:
2022-01-20 10:47:05 [INF] [logtailplugin_control.go:101] [func1] ilogtail controller is closing....:
2022-01-20 10:47:05 [INF] [logtailplugin_control.go:102] [func1] ilogtail controller is cleaning....:
2022-01-20 10:47:05 [INF] [logtailplugin_control.go:112] [func1] ilogtail controller would wait 5s to deal with the logs on the way
2022-01-20 10:47:10 [INF] [validator_control.go:89] [func1] validator controller is closing....:
2022-01-20 10:47:10 [INF] [validator_control.go:90] [func1] validator controller is cleaning....:
2022-01-20 10:47:10 [INF] [subscriber_control.go:54] [func1] subscriber controller is closing....:
2022-01-20 10:47:10 [INF] [subscriber_control.go:74] [Clean] subscriber controller is cleaning....:
2022-01-20 10:47:10 [INF] [boot_control.go:40] [func1] boot controller is stoping....:
2022-01-20 10:47:10 [INF] [boot_control.go:48] [Clean] boot controller is cleaning....:
Stopping ilogtail-e2e_ilogtail_1 ... done
Stopping ilogtail-e2e_exporter_1 ... done
Stopping ilogtail-e2e_goc_1      ... done
Removing ilogtail-e2e_ilogtail_1 ... done
Removing ilogtail-e2e_exporter_1 ... done
Removing ilogtail-e2e_goc_1      ... done
Removing network ilogtail-e2e_default
2022-01-20 10:47:11 [INF] [controller.go:112] [Start] Testing is completed:
2022-01-20 10:47:11 [INF] [controller.go:122] [Start] the E2E testing is passed:
v1.3.8
=========================================
All testing cases are passed
========================================
```

## Local Node Exporter collection

1\. Prepare the Linux environment.

2\. Download NodeExporter, download address: [https://prometheus.io/download/#node_exporter](https://prometheus.io/download/#node_exporter), and start. After startup, you can view the metrics Metrics of the NodeExporter through curl 127.0.0.1:9100/metrics.

3\. [Download](https://github.com/alibaba/ilogtail/releases) install the latest ilogtail version.

```bash
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

4\. Create a collection configuration directory.

```bash
# 1. Create sys_conf_dir
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

5\. Set the collection configuration file and write the following files into the sys_conf_dir/user_local_config.json file. The above core configuration is the plugin section. The configuration shows that we have started the Prometheus collection plug-in with the collection Port 9100,And we save the collected data in the node_exporter.log file.

```json
{
    "metrics":{
        "##1.0##k8s-log-custom-test-project-helm-0":{
            "aliuid":"1654218965343050",
            "category":"container_stdout_logstore",
            "create_time":1640692891,
            "defaultEndpoint":"cn-beijing-b-intranet.log.aliyuncs.com",
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
                            "Yaml":"global:\n  scrape_interval: 15s\n  evaluation_interval: 15s\nscrape_configs:\n  - job_name: \"prometheus\"\n    static_configs:\n      - targets: [\"localhost:9100\"]"
                        },
                        "type":"service_prometheus"
                    }
                ],
                "flushers":[
                    {
                        "detail":{
                            "FileName":"./node_exporter.log"
                        },
                        "type":"flusher_stdout"
                    }
                ]
            },
            "priority":0,
            "project_name":"k8s-log-custom-test-project-helm",
            "raw_log":false,
            "region":"cn-beijing-b",
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

6\. Start the iLogtail to view the collected data.

```bash
# Start collection
$ ./ilogtail_1.0.28
$ ps -ef|grep logtail
root       48453       1   ./ilogtail_1.0.28
root       48454   48453   ./ilogtail_1.0.28

# View collected data
tailf node_exporter.json
2022-01-20 11:38:52 {"__name__":"promhttp_metric_handler_errors_total","__labels__":"cause#$#gathering|instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"0","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"promhttp_metric_handler_requests_in_flight","__labels__":"instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"1","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"promhttp_metric_handler_requests_total","__labels__":"code#$#200|instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"1393","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"promhttp_metric_handler_requests_total","__labels__":"code#$#500|instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"0","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"promhttp_metric_handler_requests_total","__labels__":"code#$#503|instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"0","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"up","__labels__":"instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"1","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"scrape_duration_seconds","__labels__":"instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"0.011","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"scrape_samples_scraped","__labels__":"instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"1189","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"scrape_samples_post_metric_relabeling","__labels__":"instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"1189","__time__":"1642649932"}
2022-01-20 11:38:52 {"__name__":"scrape_series_added","__labels__":"instance#$#localhost:9100|job#$#prometheus","__time_nano__":"1642649932033","__value__":"0","__time__":"1642649932"}
```

## Log service NodeExporter collection

### iLogtail data collected Prometheus

1. Refer to [server environment log collection to SLS](https://github.com/alibaba/ilogtail/blob/main/docs/zh/usecases/How-to-setup-on-host.md) establish a link between the host iLogtail and Alibaba Cloud log service.
2. Host download NodeExporter, download address: [https://prometheus.io/download/#node_exporter](https://prometheus.io/download/#node_exporter), and start. After startup, you can view the metrics Metrics of the NodeExporter through curl 127.0.0.1:9100/metrics.
3. Create a log service MetricStore.! [Image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1644941045436-540723d4-9ec8-40b2-a823-395d8f4b9f14.png#clientId=u7b2622d4-68ae-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=899&id=u397c0ba6&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1798&originWidth=5078&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1687136&status=done&style=none&taskId=uf5326ee3-9abf-4992-a1b9-995f1b4dbbb&title=&width=2539)
4. Create Prometheus collection configuration.! [Image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1644941046161-d05cae9e-78ef-4868-b9eb-fc9d03933afa.png#clientId=u7b2622d4-68ae-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=1327&id=uc5d8d99f&margin=%5Bobject%20Object%5D&name=image.png&originHeight=2654&originWidth=5104&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1619794&status=done&style=none&taskId=u7590c247-e470-4f14-849c-a2f87ffd181&title=&width=2552) ![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1644941045019-d9e639e1-af34-4b1f-b493-9e907d401021.png#clientId=u7b2622d4-68ae-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=1016&id=YrIom&margin=%5Bobject%20Object%5D&name=image.png&originHeight=2032&originWidth=5110&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1157627&status=done&style=none&taskId=u83af2842-a512-46e5-aeb5-cca81d292bf&title=&width=2555)
5. View collected data

As shown in the following figure, the iLogtail Metrics collected by the NodeExporter are displayed in graphs. The query language Metrics log service is fully compatible with PromQL. For more visualization users, see [https://help.aliyun.com/document_detail/252810.html](https://help.aliyun.com/document_detail/252810.html)。
![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1644941045099-18a4e7f8-7b15-4b65-80d1-5ba482033f7e.png#clientId=u7b2622d4-68ae-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=688&id=u692a8d64&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1376&originWidth=5118&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1242472&status=done&style=none&taskId=u29f1caad-1026-412c-b72f-f19b4dad06f&title=&width=2559)
For example, calculate the SysLoad within 5 minutes:
![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1644941045122-01b62b41-7591-4942-bc93-6ba8b5945472.png#clientId=u7b2622d4-68ae-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=685&id=Jittg&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1370&originWidth=5116&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1453969&status=done&style=none&taskId=u8549f40c-102f-4b55-8b76-20381f5a75d&title=&width=2558)

### Use Grafana to connect to log service MetricStore

1. For installation Grafana, see [Grafana Installation Guide](https://grafana.com/docs/grafana/latest/installation/).
1. Use Grafana Prometheus data sources to connect to log service. For more information, see [time series data connection Grafana](https://help.aliyun.com/document_detail/173903.html).
1. Download NodeExporter Kanban Json file, such as [Node Exporter Full Kanban](https://grafana.com/grafana/dashboards/1860).
1. Import the Kanban.
![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1644941050539-053853c7-b41e-4878-9025-747bfeb4dbca.png#clientId=u7b2622d4-68ae-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=993&id=u50fa9b1d&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1986&originWidth=3582&originalType=binary&ratio=1&rotation=0&showTitle=false&size=485985&status=done&style=none&taskId=uda685e33-85bd-42a5-b971-f6bc5258f80&title=&width=1791)
1. View the metric data, as shown in the following figure, Grafana shows the metric data collected by the iLogtail.

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1644941051766-a7480574-2ed3-4045-bd4a-527814075117.png#clientId=u7b2622d4-68ae-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=1006&id=u3f66e6fb&margin=%5Bobject%20Object%5D&name=image.png&originHeight=2012&originWidth=3580&originalType=binary&ratio=1&rotation=0&showTitle=false&size=1799336&status=done&style=none&taskId=uddc25eaf-ca05-4e3c-a728-a525af4efe7&title=&width=1790)

## Summary

iLogtail provides a complete collection Prometheus metrics. You can collect Exporter metrics without modifying Prometheus metrics. You can also use log service to MetricStore capabilities as an alternative to Prometheus,Quickly build your own monitoring dashboard through the rich Kanban templates of the Grafana store.

## Reference

- [Grafana Installation Guide](https://grafana.com/docs/grafana/latest/installation/)
- [Time series data docking Grafana](https://help.aliyun.com/document_detail/173903.html)
- [iLogtail transport layer protocol](https://github.com/alibaba/ilogtail/blob/main/pkg/protocol/proto/sls_logs.proto)
- [Server environment log collection to SLS](https://github.com/alibaba/ilogtail/blob/main/docs/zh/usecases/How-to-setup-on-host.md)
