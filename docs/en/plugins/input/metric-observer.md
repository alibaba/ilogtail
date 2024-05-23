# Non-intrusive network call data

## Introduction

The 'observer_ilogtail_network' plug-in allows you to collect layer -4 network calls from network system calls and identify layer -7 network call details with the help of the network resolution module. The core network observation capability of this function is realized based on eBPF technology and has core advantages such as high performance and no intrusion.

## Version

[Beta](../stability-level.md)

## Prerequisites

1. Linux kernel 4.19 +
2. Download the libebpf.so dynamic link library and put it in the running path of the iLogtail. This code will be CoolBpf open source later.
   - `wget https://logtail-release-us-west-1.oss-us-west-1.aliyuncs.com/kubernetes/libebpf.so`

## Configure parameters

### Basic parameters

| Parameter | Type | Required | Description |
|-----------------------------| ----------------- | -------- |------------------------------------------------------------|
| Type | String | No | The plug-in Type. The value is 'observer_ilogtail_network_v1 '.                    |
| Common.FlushOutL4Interval | Number | No | Aggregation data output interval, default 15s |
| Common.FlushOutL7Interval | Number | No | Aggregation data output interval, default 60s |
| Common.FlushMetaInterval | Number | No | The interval between reading container metadata. The default value is 30 seconds. The behavior is similar to docker ps. |
| Common.FlushNetlinkInterval | Number | No | Set the time interval for reading Socket metadata. Default value: 10s |
| Common.Sampling | Number | No | Set the Sampling rate of network data. Default value: 100 |
| Common.ProtocolProcess | bool | No | When enabled, the Logtail parses network protocol data at the application layer, such as HTTP, MySQL, and Redis,Enabled by default. |
| Common.DropUnixSocket | Number | No | When enabled, Unix network requests are discarded. Unix domains are often used for local network interaction, which is enabled by default. |
| Common.DropLocalConnections | Number | No | Discards a network request in the INET domain whose peer address is local after it is enabled. It is enabled by default. |
| Common.DropUnknownSocket | Number | No | When enabled, network requests from non-INET or Unix domains are discarded. This parameter is enabled by default. |
| Common.IncludeProtocols | []string | No | The protocol type for layer -7 network protocol identification. The default value is all. Currently, HTTP, Redis, MySQL, PgSQL, and DNS are supported. |
| Common.Tags | map[string]string | No | tagb Tags will be uploaded with the tag. |
| EBPF.Enabled                | map[string]string | 是       | 开启Ebpf 功能                                                  |

### Advanced parameters

| Parameter | Type | Required | Description |
| -------------------------------- | ----------------- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| Common.IncludeContainerLabels | map[string]string | No | Specifies the container to be collected. The relationship between multiple whitelists is OR, that is, the container Label can be matched as long as it meets any whitelist.Note: This variable is a container Label, not a Kubernetes Label |
| Common.ExcludeContainerLabels | map[string]string | No | Is used to exclude containers to be collected. The relationship between multiple blacklists is OR, that is, the container Label can be matched as long as it meets any blacklist.Note: This variable is a container Label, not a Kubernetes Label |
| Common.IncludeK8sLabels | map[string]string | No | Specifies the container to be collected. Set LabelKey to a specific name and LabelValue to a regular expression.例如设置LabelKey为io.kubernetes.container.name，设置LabelValue为^(nginx     |
| Common.ExcludeK8sLabels | map[string]string | No | Is used to exclude containers that do not need to be collected. Set LabelKey to a specific name and LabelValue to a regular expression.例如设置LabelKey为io.kubernetes.container.name，设置LabelValue为^(nginx |
| Common.IncludeEnvs | map[string]string | No | Specifies the container to be collected. Set EnvKey as the specific name and EnvValue as a regular expression.For example, set EnvKey to NGINX_SERVICE_PORT and EnvValue to ^(80 |
| Common.ExcludeEnvs | map[string]string | No | Is used to exclude containers that do not need to be collected. Set EnvKey as the specific name and EnvValue as a regular expression.For example, set EnvKey to NGINX_SERVICE_PORT and EnvValue to ^(80 |
| Common.IncludeCmdRegex | string | No | Sets a regular expression on the command line to specify the processes to be monitored. For example, set it to ^ mockclient$,Indicates that only processes named cmdline under/proc/{pid}/mockclient are monitored. |
| Common.ExcludeCmdRegex | string | No | Sets a regular expression on the command line to exclude processes that do not require monitoring. For example, set it to ^ mockclient$,The process named cmdline in/proc/{pid}/mockclient is not monitored. |
| Common.IncludeContainerNameRegex | string | No | Enter a regular expression that matches the Contianer name to specify the Contianer to be collected.                                                                                   |
| Common.ExcludeContainerNameRegex | string | No | Enter a regular expression that matches the name of the Contianer to exclude Contianer that do not need to be collected.                                                                               |
| Common.IncludePodNameRegex | string | No | Enter a regular expression that matches the Pod name to specify the Pod to be collected. |
| Common.ExcludePodNameRegex | string | No | Enter a regular expression that matches the Pod name to exclude pods that do not need to be collected. |
| Common.IncludeNamespaceNameRegex | string | No | Enter a regular expression that matches the Namespace name to specify the Namespace to be collected.                                                                                     |
| Common.ExcludeNamespaceNameRegex | string | No | Enter a regular expression that matches the name of the Namespace to exclude namespaces that do not need to be collected.                                                                                 |
| EBPF.Pid | map[string]string | No | Specify a unique process ID for collection range determination |

## Example: collect HTTP request calls

### Collection configuration

observer_ilogtail_network_v1 program runs in the core part of C ++. You need to add the processor_default plug-in to select multiple output plug-ins in the community, such as Kafka *

```yaml
enable: true
inputs:
  - Type: observer_ilogtail_network_v1
    Common:
      FlushOutL4Interval: 5
      FlushOutL7Interval: 5
    EBPF:
      Enabled: true
processors:
  - Type: processor_default
flushers:
  - Type: flusher_stdout
```

#### Output

```plain
2022-08-25 16:44:01 [INF] [flusher_stdout.go:119] [Flush] [config#/root/workspace/ilogtail/output/./config/local/demo.yaml,] {"type":"protocols","_process_pid_":"605","_process_cmd_":"/usr/sbin/nscd","_running_mode_":"host","protocol":"dns","query_record":"ilogtail-community-edition.cn-shanghai.log.aliyuncs.com","query_type":"A","success":"1","role":"client","remote_info":"{\"remote_ip\":\"100.100.2.136\",\"remote_port\":\"53\",\"remote_type\":\"dns\"}\n","total_count":"1","total_latency_ns":"387138","total_req_bytes":"73","total_resp_bytes":"272","__time__":"1661417038"}:
2022-08-25 16:44:01 [INF] [flusher_stdout.go:119] [Flush] [config#/root/workspace/ilogtail/output/./config/local/demo.yaml,] {"type":"protocols","_process_pid_":"1945","_process_cmd_":"/usr/local/cloudmonitor/bin/argusagent","_running_mode_":"host","protocol":"http","version":"1","host":"metrichub-cn-beijing.aliyun.com","method":"POST","url":"/agent/metrics/putLines","resp_code":"200","role":"client","remote_info":"{\"remote_ip\":\"100.100.105.70\",\"remote_port\":\"80\",\"remote_type\":\"server\"}\n","total_count":"1","total_latency_ns":"3492453","total_req_bytes":"13661","total_resp_bytes":"165","__time__":"1661417038"}:
2022-08-25 16:44:01 [INF] [flusher_stdout.go:119] [Flush] [config#/root/workspace/ilogtail/output/./config/local/demo.yaml,] {"type":"statistics","_process_pid_":"1945","_process_cmd_":"/usr/local/cloudmonitor/bin/argusagent","_running_mode_":"host","remote_info":"{\"remote_ip\":\"100.100.105.70\",\"remote_port\":\"80\",\"remote_type\":\"server\"}\n","socket_type":"inet_socket","role":"client","send_bytes":"13661","recv_bytes":"165","send_packets":"1","recv_packets":"1","send_total_latency":"0","recv_total_latency":"0","__time__":"1661417038"}:
2022-08-25 16:44:01 [INF] [flusher_stdout.go:119] [Flush] [config#/root/workspace/ilogtail/output/./config/local/demo.yaml,] {"type":"statistics","_process_pid_":"605","_process_cmd_":"/usr/sbin/nscd","_running_mode_":"host","remote_info":"{\"remote_ip\":\"100.100.2.136\",\"remote_port\":\"53\",\"remote_type\":\"dns\"}\n","socket_type":"inet_socket","role":"client","send_bytes":"73","recv_bytes":"272","send_packets":"1","recv_packets":"1","send_total_latency":"0","recv_total_latency":"0","__time__":"1661417038"}:
```
