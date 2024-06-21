# iuput_ebpf_file_observer 插件

## 简介

`iuput_ebpf_file_observer`插件可以实现利用ebpf探针采集文件可观测数据。

## 版本

[Alpha](../stability-level.md)

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为iuput\_ebpf\_file\_observer  |
|  ProbeConfig  |  object  |  是  |  /  |  插件配置参数列表  |
|  ProbeConfig.ProfileRemoteServer  |  string  |  否  |  空  |  TODO  |
|  ProbeConfig.CpuSkipUpload  |  bool  |  否  |  false  |  TODO  |
|  ProbeConfig.MemSkipUpload  |  bool  |  否  |  false  |  TODO  |

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
  - Type: input_ebpf_profilingprobe_observer
    ProbeConfig:
      ProfileRemoteServer: "remote-server"
      CpuSkipUpload: false
      MemSkipUpload: false
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: 
```

* 输出

```json
TODO
```
