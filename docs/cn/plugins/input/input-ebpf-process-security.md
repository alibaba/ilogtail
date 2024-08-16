# input_ebpf_process_security 插件

## 简介

`input_ebpf_process_security`插件可以实现利用ebpf探针采集进程安全相关动作。

## 版本

[Dev](../stability-level.md)

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为input\_ebpf\_process\_security  |
|  ProbeConfig  |  \[object\]  |  是  |  /  |  插件配置参数列表  |
|  ProbeConfig.CallName  |  \[string\]  |  是  |  空  |  内核挂载点  |

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
  - Type: input_ebpf_processprobe_security
    ProbeConfig:
      - CallName: 
        - "sys_enter_execve"
        - "sys_enter_clone"
        - "disassociate_ctty"
        - "acct_process"
        - "wake_up_new_task"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* 输出

```json
TODO
```
