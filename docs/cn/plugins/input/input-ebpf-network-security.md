# input_ebpf_network_security 插件

## 简介

`input_ebpf_network_security`插件可以实现利用ebpf探针采集网络安全相关动作。

## 版本

[Alpha](../stability-level.md)

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为iuput\_ebpf\_network\_security  |
|  ConfigList  |  \[object\]  |  是  |  /  |  插件配置参数列表  |
|  ConfigList.CallName  |  \[string\]  |  否  |  空  |  系统调用函数  |
|  ConfigList.Filter  |  object  |  是  |  /  |  过滤参数  |
|  ConfigList.Filter.DestAddrList  |  \[string\]  |  否  |  空  |  目的IP地址  |
|  ConfigList.Filter.DestPortList  |  \[string\]  |  否  |  空  |  目的端口  |
|  ConfigList.Filter.DestAddrBlackList  |  \[string\]  |  否  |  空  |  目的IP地址黑名单  |
|  ConfigList.Filter.DestPortBlackList  |  \[string\]  |  否  |  空  |  目的端口黑名单  |
|  ConfigList.Filter.SourceAddrList  |  \[string\]  |  否  |  空  |  源IP地址  |
|  ConfigList.Filter.SourcePortList  |  \[string\]  |  否  |  空  |  源端口  |
|  ConfigList.Filter.SourceAddrBlackList  |  \[string\]  |  否  |  空  |  源IP地址黑名单  |
|  ConfigList.Filter.SourcePortBlackList  |  \[string\]  |  否  |  空  |  源端口黑名单  |

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
    ConfigList:
      - CallName: 
        - "tcp_connect"
        - "tcp_close"
        Filter: 
          DestAddrList: 
            - "10.0.0.0/8"
            - "92.168.0.0/16"
          DestPortList: 
            - 80
          SourceAddrBlackList: 
            - "127.0.0.1/8"
          SourcePortBlackList: 
            - 9300
      - CallName: 
        - "tcp_sendmsg"
        Filter: 
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
