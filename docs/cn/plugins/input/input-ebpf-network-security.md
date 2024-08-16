# input_ebpf_network_security 插件

## 简介

`input_ebpf_network_security`插件可以实现利用ebpf探针采集网络安全相关动作。

## 版本

[Dev](../stability-level.md)

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为input\_ebpf\_network\_security  |
|  ProbeConfig  |  \[object\]  |  否  |  /  |  插件配置参数列表  |
|  ProbeConfig.CallNameFilter  |  \[string\]  |  否  |  空  |  内核挂载点过滤器，按照白名单模式运行，不填表示配置该插件所支持的所有挂载点  |
|  ProbeConfig.AddrFilter  |  object  |  否  |  /  |  网络地址过滤器  |
|  ProbeConfig.AddrFilter.DestAddrList  |  \[string\]  |  否  |  空  |  目的IP地址白名单  |
|  ProbeConfig.AddrFilter.DestPortList  |  \[string\]  |  否  |  空  |  目的端口白名单  |
|  ProbeConfig.AddrFilter.DestAddrBlackList  |  \[string\]  |  否  |  空  |  目的IP地址黑名单  |
|  ProbeConfig.AddrFilter.DestPortBlackList  |  \[string\]  |  否  |  空  |  目的端口黑名单  |
|  ProbeConfig.AddrFilter.SourceAddrList  |  \[string\]  |  否  |  空  |  源IP地址白名单  |
|  ProbeConfig.AddrFilter.SourcePortList  |  \[string\]  |  否  |  空  |  源端口白名单  |
|  ProbeConfig.AddrFilter.SourceAddrBlackList  |  \[string\]  |  否  |  空  |  源IP地址黑名单  |
|  ProbeConfig.AddrFilter.SourcePortBlackList  |  \[string\]  |  否  |  空  |  源端口黑名单  |

## 样例

### XXXX

* 输入

```json
TODO
```

* 采集配置

```yaml
enable: true
inputs:
  - Type: input_ebpf_sockettraceprobe_security
    ProbeConfig:
      - CallNameFilter: 
        - "tcp_connect"
        - "tcp_close"
        AddrFilter: 
          DestAddrList: 
            - "10.0.0.0/8"
            - "92.168.0.0/16"
          DestPortList: 
            - 80
          SourceAddrBlackList: 
            - "127.0.0.1/8"
          SourcePortBlackList: 
            - 9300
      - CallNameFilter: 
        - "tcp_sendmsg"
        AddrFilter: 
          DestAddrList: 
            - "10.0.0.0/8"
            - "92.168.0.0/16"
          DestPortList: 
            - 80
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* 输出

```json
TODO
```
