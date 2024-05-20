# Compare the performance of iLogtail and Filebeat in container scenarios

## Preface

Some time ago, iLogtail([https://github.com/alibaba/ilogtail](https://github.com/alibaba/ilogtail)) the observable collector of Alibaba tens of millions of instances is open-source. It introduces that the collection performance of iLogtail can reach 100 MB/s per single core, which has 5-10 times the performance advantage compared with the open-source collection Agent. Many partners iLogtail curious about the specific performance data and resource consumption. This article will compare the Agent FileBeat with high usage and relatively good performance in the industry,Test the performance of the two agents in different stress scenarios.

## Test description

With the popularization Kubernetes, the requirements of log collection in Kubernetes are becoming more and more normal. Therefore, the following sections will respectively conduct the comparison test of container standard output stream collection and static file collection in containers (for those who use static file collection, please refer to the comparison test of static file collection in containers. iLogtail, pure static file collection is slightly better than test 2). The test items are as follows:

- **Experiment 1**: constant collection configuration 4,Filebeat and iLogtail performance comparison of **standard output stream collection** at raw log generation rates of 1 m/s, 2 m/s, and 3 m/s.
- **Experiment 2**: constant collection configuration 4,Filebeat & iLogtail the performance comparison of **File Collection in containers** at the raw log generation rate of 1 m/s, 2 m/s, and 3 m/s.

In a real production environment, the O & M of log collection components is also crucial. For the convenience of O & M and post-upgrade, it is more common to use **Sidecar mode** to deploy collection components in K8s than in Daemonset mode. However, due to the feature that **Daemonset** distribute the collection configuration of the entire cluster to each collection node at the same time,The number of working configurations of a single collection node must be smaller than the number of full collection configurations. Therefore, we will also conduct the following two tests to verify whether the expansion of the collection configuration will affect the efficiency of the collector:

- **Experiment 3**: the constant input rate is 3 m/s, Filebeat & iLogtail the performance comparison of **standard output stream collection** under the collection configuration of 50, 100, 500, and 1000 copies.
- **Experiment 4**: the constant input rate is 3 m/s, Filebeat & iLogtail the performance comparison of **File Collection in containers** with 50, 100, 500, and 1000 collection configurations.

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537108680-be968e23-750e-4aae-a63d-61ec7d881dbd.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&id=N2pXy&margin=%5Bobject%20Object%5D&name=image.Png&originHeight=400&originWidth=500&originalType=url&ratio=1&rotation=0&showTitle=False&size=42799&status=done&style=none&taskId=uf19fc34e-90ff-4e3b-908a-9abf6dbd4cb&title=)
Finally, a iLogtail high-traffic stress test will be conducted, as follows:

- **Experiment 5**:iLogtail **standard output stream collection** performance at 5 m/s, 10 m/s, 10 m/s, and 40 m/s.
- **Experiment 5**:iLogtail the performance of **File Collection in containers** at 5 m/s, 10 m/s, 10 m/s, and 40 m/s.

## Test Environment

All collection environment data is stored in [https://github.com/EvanLjp/ilogtail-comparison](https://github.com/EvanLjp/ilogtail-comparison), interested students can do the whole comparison test by themselves. The following sections describe the specific configuration of different collection modes respectively. If you only care about the collection comparison results, you can skip this section and continue reading.

### Environment

Runtime Environment: Alibaba Cloud ACK Pro
Node configuration: ecs.g6.xlarge (4 vCPU 16GB) disk ESSD
Underlying container: Containerd
iLogtail version: 1.0.28
FileBeat version: v7.16.2

### Data source

For data sources, we first remove the differences caused by regular parsing or multi-line splicing capabilities, and only compare them with the most basic single-line collection. The data generation source simulates to generate nginx access logs. The size of a single log is 283B. The following configuration describes the input sources at a rate of 1,000 entries per second:

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: nginx-log-demo-0
  namespace: default
spec:
  template:
    metadata:
      name: nginx-log-demo-0
    spec:
      restartPolicy: Never
      containers:
        - name: nginx-log-demo-0
          image: registry.cn-hangzhou.aliyuncs.com/log-service/docker-log-test:latest
          command: ["/bin/mock_log"]
          args:  ["--log-type=nginx",  "--path=/var/log/medlinker/access.log", "--total-count=1000000000", "--log-file-size=1000000000", "--log-file-count=2", "--logs-per-sec=1000"]
          volumeMounts:
            - name: path
MountPath: /var/log/medlinker
              subPath: nginx-log-demo-0
          resources:
            limits:
              memory: 200Mi
            requests:
              cpu: 10m
              memory: 10Mi
      volumes:
      - name: path
        hostPath:
          path: /testlog
          type: DirectoryOrCreate
      nodeSelector:
        kubernetes.io/hostname: cn-beijing.192.168.0.140
```

### Filebeat standard output stream collection configuration

Filebeat supports container file collection in native mode. The add_kubernetes_metadata component is used to add kubernetes metadata. To avoid performance differences caused by output components, the drop_event plug-in is used to discard data to avoid output,filebeat the test configuration is as follows (harvester_buffer_size is adjusted to 512K,filebeat.registry.flush: 30s, and the queue.mem parameter is appropriately enlarged to increase throughput):

```yaml
Filebeat. yml: |-
    filebeat.registry.flush: 30s
    processors:
-Add_kubernetes_metadata:
          host: ${NODE_NAME}
          matchers:
          - logs_path:
Logs_path: "/var/log/containers/"
      - drop_event:
          when:
            equals:
              input.type: container
    output.console:
      pretty: false
Tail:
      mem:
        events: 4096
        flush.min_events: 2048
        flush.timeout: 1s
    max_procs: 4
    filebeat.inputs:
    - type: container
      harvester_buffer_size: 524288
      paths:
        - /var/log/containers/nginx-log-demo-0-*.log
```

### Filebeat container file collection configuration

Filebeat does not support file collection in containers, you need to manually mount the log print path to the host HostPath. Here, we use the subPath and DirectoryOrCreate functions to separate the Service print path. The following is a simulation of the independent service log print path.
![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537109076-1a362223-c863-4887-996b-c56b90e9f500.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&id=ucbabc4ee&margin=%5Bobject%20Object%5D&name=image.Png&originHeight=272&originWidth=3044&originalType=url&ratio=1&rotation=0&showTitLe=false&size=419109&status=done&style=none&taskId=u51d1cf1cc-27d1-4d8c-a95d-9e0053a8cd1&title=)
filebeat use the basic log reading function to read/testlog logs in the path. To avoid performance differences caused by output components, the drop_event plug-in discards data to avoid output. The test configuration is as follows (harvester_buffer_size is adjusted to 512K,filebeat.registry.flush: 30s, the queue.mem parameter is appropriately enlarged to increase throughput):

```yaml
Filebeat. yml: |-
    filebeat.registry.flush: 30s
    output.console:
      pretty: false
Tail:
      mem:
        events: 4096
        flush.min_events: 2048
        flush.timeout: 1s
    max_procs: 4
    
    filebeat.inputs:
    - type: log
      harvester_buffer_size: 524288
      paths:
        - /testlog/nginx-log-demo-0/*.log
      processors:
        - drop_event:
            when:
              equals:
                log.file.path: /testlog/nginx-log-demo-0/access.log
```

### iLogtail standard output stream collection configuration

iLogtail the native supports standard output stream collection, the service_docker_stdout component has already extracted kubernetes meta information. To avoid performance differences caused by the output component, the processor_filter_regex,Filter all logs. The test configuration is as follows:

```yaml
{
    "inputs":[
        {
            "detail":{
                "ExcludeLabel":{

                },
                "IncludeLabel":{
                    "io.kubernetes.container.name":"nginx-log-demo-0"
                }
            },
            "type":"service_docker_stdout"
        }
    ],
    "processors":[
        {
            "type":"processor_filter_regex",
            "detail":{
                "Exclude":{
                    "_namespace_":"default"
                }
            }
        }
    ]
}
```

### iLogtail container file collection configuration

iLogtail supports file collection in containers. However, because the metadata collected in files exists in tag tags, no filter plug-in is available. To avoid performance differences caused by output components, we use an empty output plug-in for output. The test configuration is as follows:

```yaml
{
    "metrics":{
        "c0":{
            "advanced":{
                "k8s":{
                    "IncludeLabel":{
                        "io.kubernetes.container.name":"nginx-log-demo-0"
                    }
                }
            },
           ......
            "plugin":{
                "processors":[
                    {
                        "type":"processor_default"
                    }
                ],
                "flushers":[
                    {
                        "type":"flusher_statistics",
                        "detail":{
                            "RateIntervalMs":1000000
                        }
                    }
                ]
            },
            "local_storage":true,
"Log_begin_reg":".*",
"Log_path":"/var/log/medlinker"
            ......
        }
    }
}
```

## Comparative test of Filebeat and iLogtail

The comparison items between Filebeat and iLogtail include the following: standard output stream collection performance, container file collection performance, standard output stream multi-user configuration performance, container file multi-user configuration performance, and high-traffic collection performance.

### Comparison of standard output stream collection performance

Input data source: 283B/s, bottom container contianerd, standard output stream after expansion 328B, a total of 4 input sources:

- 1 m/s, 3700 input logs/s,
- 2 m/s, 7400 input logs/s,
- 3 m/s input logs: 11100/s.

The following shows the performance comparison of different collections of standard output streams. It can be seen that iLogtail has ten times the performance advantage compared with Filebeat (CPU percentage is single core percentage):

|  | Filebeat | ILogtail |
| --- | --- | --- |
| 1M/s | 20% | 2% |
| 2M/s | 37% | 3.3% |
| 3M/s | 52% | 4.1% |

The following shows the memory comparison of different collections of standard output streams. It can be seen that the overall memory difference between logtail and filebeat is not large, and there is no memory surge as the collection traffic increases:

|  | Filebeat | ILogtail |
| --- | --- | --- |
| 1M/s | 136M | 90M |
| 2M/s | 134M | 90M |
| 3M/s | 144M | 90M |

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537108630-b5df6ebb-79fd-4ee9-ad03-066d500fd92f.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=u8b666ff1&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=24975&status=done&style=none&taskId=uef68e6c0-1f70-4ccd-afb4-64a56646a46&title=&width=362)![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537108684-5cc7cff8-a68b-4a78-835d-d4429455f9a3.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=uf0f8a5a2&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=22498&status=done&style=none&taskId=ueff2dae5-1662-4006-b50b-524bbb377da&title=&width=362)

### Comparison of file collection performance in containers

Input data source: 283B/s, a total of 4 input sources:

- 1 m/s, 3700 input logs/s,
- 2 m/s, 7400 input logs/s,
- 3 m/s input logs: 11100/s.

The following shows the performance comparison of different collection of files in containers. Filebeat, files in containers share collection components with container collection, and Kubernets meta-related components are omitted. Therefore, compared with standard output stream collection, iLogtail collection of files in containers uses Polling + [inotify](https:// man7.org/linux/man-pages/man7/inotify.7.html) mechanism also improves performance compared with container standard output stream collection, but it can be seen that iLogtail has five times the performance advantage compared with Filebeat (CPU percentage is single-core percentage):

|  | Filebeat | ILogtail |
| --- | --- | --- |
| 1M/s | 5% | 1.3% |
| 2M/s | 11% | 2% |
| 3M/s | 15% | 3% |

The following shows the memory comparison of different collections of standard output streams. It can be seen that the overall memory difference between logtail and filebeat is not large, and there is no memory surge as the collection traffic increases:

|  | Filebeat | ILogtail |
| --- | --- | --- |
| 1M/s | 136M | 126M |
| 2M/s | 140M | 141M |
| 3M/s | 141M | 142M |

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537108662-06fc93ab-8f7a-4445-873b-fbd68a21532e.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=uda491b86&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=26536&status=done&style=none&taskId=uad7154f3-afb4-40fc-8028-f7c4ee01250&title=&width=362)![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537110162-8f28dba5-6e55-4d0e-81fe-5640f2b0d225.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=u96a02dca&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=23282&status=done&style=none&taskId=u4c18e606-50b4-4b47-8663-6f3baaf0c0b&title=&width=362)

### Comparison of expansion performance of collection configuration

The expansion performance of the acquisition configuration is compared. The input source is set to 4 and the total input rate is 3 m/s. The acquisition configuration is 50, 100, 500 and 1000 respectively.

#### Standard output stream collection configuration expansion comparison

The following shows the performance comparison of different collection of standard output streams. As the underlying layer of container collection and static file collection share the same static file collection logic, Filebeat will have a lot of regular matching work under the standard output stream collection path var/log/containers,It can be seen that although the amount of collected data does not increase, the CPU consumption increases by 10% + due to the increase of collection configuration. However, iLogtail share the container path discovery mechanism for the container collection model globally, the performance loss caused by regular logic is avoided (the percentage of CPU is the percentage of single core).

| Collection configuration | Filebeat | ILogtail |
| --- | --- | --- |
| 50 | 65% | 5% |
| 100 | 68% | 6.30% |
| 500 | 75% | 7% |
| 1000 | 82% | 8.70% |

In terms of memory expansion, it can be seen that both Filebeat and iLogtail have memory expansion caused by the increase of acquisition configuration, but the expansion size of both is within an acceptable range.

| Collection configuration | Filebeat | ILogtail |
| --- | --- | --- |
| 50 | 148M | 95M |
| 100 | 149M | 98M |
| 500 | 163M | 115M |
| 1000 | 177M | 149M |

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537110426-67cc2eb3-9ee1-4a85-9dc4-b0fb7c945f2f.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=ua9f978a4&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=26052&status=done&style=none&taskId=ub714d946-c43f-4ae6-beb1-31499cbb131&title=&width=362)![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537110719-bacb6b41-6c46-47c1-96ef-c4c4f38d5980.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=u6c79dee0&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=24507&status=done&style=none&taskId=u7ae91542-6b57-4d12-841a-d48f99fb5e2&title=&width=362)

#### Comparison of file collection configuration expansion in containers

The following shows the performance comparison of different collectors for file collection in containers. It can be seen that Filebeat static file collection avoids regular path usage for standard output circulation. Compared with the standard, the CPU consumption is less, and the iLogtail CPU change is also very small. Compared with the standard output stream collection, the performance is slightly better (CPU percentage is the percentage of single core).

| Collection configuration | Filebeat | ILogtail |
| --- | --- | --- |
| 50 | 16% | 4.30% |
| 100 | 16.30% | 4.30% |
| 500 | 20.00% | 5.60% |
| 1000 | 22% | 7.30% |

In terms of memory expansion, it can also be seen that both Filebeat and iLogtail have memory expansion caused by the increase of acquisition configuration, but the expansion size of both is within an acceptable range.

| Collection configuration | Filebeat | ILogtail |
| --- | --- | --- |
| 50 | 142M | 99M |
| 100 | 145M | 104M |
| 500 | 164M | 128M |
| 1000 | 183M | 155M |

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537111346-e42487d7-27f7-4fe7-8a02-2fc2fdceb1de.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=u6cc33de6&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=21701&status=done&style=none&taskId=u6280f78e-b3a3-4f53-a32e-dc954121784&title=&width=362)![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537111701-31115936-caf3-4d7b-a3ae-f82b62c8b2cd.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=217&id=uf56cd907&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=24544&status=done&style=none&taskId=uc83f0fe5-42ad-4375-8c7d-a0a56f87004&title=&width=362)

### iLogtail collection performance test

Due to the problem of collection delay in scenarios where the FileBeat volume is large, the following scenarios only test the performance of container standard output stream collection and container file collection for iLogtail at 5 m/s, 10 m/s, and 20 m/s respectively.

- Number of input sources: 10
- Single log size 283B
- 5 m/s corresponds to a log rate of 18526 entries/s and a single input source generation rate of 1852 entries/s.
- 10 m/s corresponds to a log rate of 37052 entries/s and a single input source generation rate of 3705 entries/s.
- 20 m/s corresponds to a log rate of 74104 entries/s and a single input source generation rate of 7410 entries/s.
- 40 m/s corresponds to a log rate of 148208 entries/s and a single input source generation rate of 14820 entries/s.

Similar to the preceding tests, we can see that container file collection is slightly better than container standard output stream collection in terms of CPU consumption (CPU percentage is single core percentage), mainly due to the underlying Polling of container file collection + [inotify](https:// man7.org/linux/man-pages/man7/inotify.7.html) mechanism. Interested students can read [Logtail technology sharing 1](https://zhuanlan.zhihu.com/p/29303600) for more information.

| Rate | Container file collection | Standard output stream collection |
| --- | --- | --- |
| 5M/s | 5.70% | 4.70% |
| 10M/s  | 10.00% | 10.30% |
| 20M/s | 15.00% | 18.70% |
| 40M/s | 22% | 36% |

In terms of memory, standard output stream collection mainly depends on GO, while container file collection mainly depends on C. Due to the existence of GC mechanism, the memory consumed by standard output stream collection will gradually exceed the memory consumed by container file collection as the speed increases.

| Rate | Container file collection | Standard output stream collection |
| --- | --- | --- |
| 5M/s | 148M | 100M |
| 10M/s  | 146M | 130M |
| 20M/s | 147M | 171M |
| 40M/s | 150M | 220M |

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537111882-b3e72905-fc1c-4e45-a814-ed2e9ea233c6.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=220&id=SrpNQ&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=35015&status=done&style=none&taskId=uafd24254-727c-47d3-ba01-346d31db1c6&title=&width=367)![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537112747-026fa4e7-7c6e-4b7c-a202-a42f79e9b392.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=219&id=IrU87&margin=%5Bobject%20Object%5D&name=image.png&originHeight=434&originWidth=724&originalType=binary&ratio=1&rotation=0&showTitle=false&size=27031&status=done&style=none&taskId=uf302707e-c322-44ee-ad9d-014642dd5c4&title=&width=365)

### Comparison and summary

| | CPU comparison conclusion | Memory comparison conclusion |
| --- | --- | --- |
| Container standard output stream collection performance | If you input the same traffic, the performance of the iLogtail is ten times higher than that of the Filebeat | The memory consumption of the iLogtail is not much different from that of the Filebeat. |
| Container file collection performance | If you input the same traffic, the performance of the iLogtail is five times higher than that of the Filebeat | The memory consumption of the iLogtail is not much different from that of the Filebeat. |
| Multi-configuration performance of container standard output stream collection | Input the same traffic. As the collection configuration increases, the Filebeat CPU increase is three times of the iLogtail CPU increase. | Both iLogtail and Filebeat will cause memory expansion due to the increase of collection configuration,All are within the acceptable range. |
| Multi-configuration performance of container file collection | Enter the same traffic. As the collection configuration increases, the Filebeat CPU increase is twice the iLogtail CPU increase. | Both iLogtail and Filebeat will cause memory expansion due to the increase of collection configuration,All are within the acceptable range. |

## **Why is there a huge difference between the standard output of Filebeat containers and file collection?**

The preceding test shows that Filebeat have large CPU differences in different operating modes. The following flame diagram can be obtained through pprof collected by dump container standard output stream. It can be seen that the add_kubernetes_metadata plug-in collected by Filebeat container is a performance bottleneck,At the same time, the add_kubernetes_metadata Filebeat is in api-server same mode as that of each node, which also causes api-server pressure problems.
![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537116957-d00ae947-476c-45a8-9c14-3335231ac546.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=552&id=ua052289e&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1104&originWidth=2406&originalType=binary&ratio=1&rotation=0&showTitle=false&size=2258463&status=done&style=none&taskId=u20d727bb-d943-4bd9-ae54-c3e76ef8e6d&title=&width=1203)
However, iLogtail kubernetes meta is fully compatible with kubernetes CRI protocol, which directly reads meta data through kubernets sandbox, ensuring high-performance collection efficiency of iLogtail.

### ![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537114447-5ac3bcb9-116d-4bff-b2c0-d55be32ca939.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&id=ud0e5bd3d&margin=%5Bobject%20Object%5D&name=image.png&originHeight=136&originWidth=572&originalType=url&ratio=1&rotation=0&showTitle=false&size=27722&status=done&style=none&taskId=u6e4121cd-0024-42dc-a6cf-411f369c34c&title=)

## **iLogtail DaemonSet scenario optimization**

Through the above comparison, we can see that iLogtail has excellent memory and CPU consumption compared with Filebeat. Some partners may be curious about iLogtail reasons behind such excellent performance. The following section mainly explains the optimization of iLogtail Daemonset scenarios,How can the standard output stream have 10 times performance compared with Filebeat? For other specific optimization measures, please refer to:[Logtail technology sharing 1](https://zhuanlan.zhihu.com/p/29303600)[with Logtail technology sharing 2](https://www.Sohu.com/a/205324880_465959) 。
For standard output stream scenarios, compared with other open source collectors, such as Filebeat or Fluentd. Generally, container standard output stream files are collected by listening to var/log/containers **or**/var/log/pods,For example, the path structure of /var/log/pods/<namespace>_<pod_name>_<pod_id>/<container_name>/. This path is used to reuse the static file collection mode of the physical machine for collection.
![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537114938-4e5607fc-25c3-4059-a9c9-722f402912c6.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&id=u804ad961&margin=%5Bobject%20Object%5D&name=image.Png&originHeight=272&originWidth=2268&originalType=url&ratio=1&rotation=0&showTitle=false&size=7353&status=done&style=none&taskId=udbf1b619-feca-4c97-acac-f56eda1c085&title=)
For iLogtail, full support for Containerization is achieved. iLogtail, through the discovery mechanism, the container list of Node nodes is maintained globally, and the container list is monitored and maintained in real time. When we have a container list, we have the following advantages:

- The Collection path does not depend on the static configuration path. You can dynamically select the collection source based on the container label to simplify user access costs.
- You can detect the dynamic path of the container Mount node based on the container meta information. Therefore, iLogtail can collect files in the container without mounting. Collectors such as Filebeat need to mount the path in the container to the host path for static file collection.
- For the new access collection configuration, reuse the historical container list to quickly access the collection. For the empty collection configuration, because the container discovers the existence of the global sharing mechanism, it avoids the existence of the empty rotation listening path mechanism, thus ensuring that in an extremely dynamic environment such as containers, the cost of iLogtail O & M is controllable.

![image.png](https://cdn.nlark.com/yuque/0/2022/png/22279413/1641537116676-044a74eb-8a43-4a49-a19b-70fd0433c79d.png#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&id=ue3871d91&margin=%5Bobject%20Object%5D&name=image.png&originHeight=1606&originWidth=2898&originalType=url&ratio=1&rotation=0&showTitle=false&size=1817942&status=done&style=none&taskId=ud464f5c9-c2f7-43ef-aebb-33efd615b27&title=)

## **Conclusion**

To sum up, in a highly dynamic Kubernetes environment, iLogtail will not cause a large memory expansion due to the multi-configuration problems caused by the Daemonset deployment model. In addition, iLogtail has a performance advantage of about 5 times in static file collection,For standard output stream collection, iLogtail has a performance advantage of about 10 times due to iLogtail collection mechanism. However, compared with old open-source products such as Filebeat and Fluentd, there are still a lot of deficiencies in document construction and community construction. Friends who are interested in iLogtail are welcome to join us,Create an easy-to-use and high-performance [iLogtail](https://github.com/alibaba/ilogtail) product.
![image.png](https://sls-opensource.oss-us-west-1.aliyuncs.com/ilogtail/ilogtail-contact.png?versionId=CAEQOhiBgICQkM6b8xciIDcxZTU5M2FjMDAzODQ1Njg5NjI3ZDc4M2FhOTZkNWNk#clientId=uc89d3166-a47e-4&crop=0&crop=0&crop=1&crop=1&from=paste&id=u5ada78db&margin=%5Bobject%20Object%5D&name=image.png&originHeight=708&originWidth=8014&originalType=url&ratio=1&rotation=0&showTitle=false&size=910470&status=done&style=none&taskId=u41750091-879b-49c9-8d6c-3264841a093&title=)

## **References**

- [Logtail technology sharing I](https://zhuanlan.zhihu.com/p/29303600)
- [Logtail technology sharing II](https://www.sohu.com/a/205324880_465959)
- [Filebeat configuration](https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-input-container.html)
- [Filebeat containerized deployment](https://www.elastic.co/guide/en/beats/filebeat/current/running-on-kubernetes.html)
- [iLogtail User Guide](https://github.com/alibaba/ilogtail/blob/main/docs/zh/setup/README.md)
