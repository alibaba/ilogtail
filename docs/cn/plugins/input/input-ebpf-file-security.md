# iuput_ebpf_file_security 插件

## 简介

`iuput_ebpf_file_security`插件可以实现利用ebpf探针采集文件安全相关动作。

## 版本

[Dev](../stability-level.md)

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为input\_ebpf\_file\_security  |
|  ProbeConfig  |  \[object\]  |  是  |  /  |  插件配置参数列表  |
|  ProbeConfig.CallName  |  \[string\]  |  是  |  空  |  内核挂载点  |
|  ProbeConfig.FilePathFilter  |  \[string\]  |  否  |  空  |  使用文件路径以及文件名作为过滤参数，例如 "/etc/passwd"，遵循前缀匹配的原则  |

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
  - Type: input_ebpf_fileprobe_security
    ProbeConfig:
      - CallName: 
        - "security_file_permission"
        FilePathFilter: 
          - "/etc/passwd"
          - "/lib"
      - CallName: 
        - "security_path_truncate"
        FilePathFilter: 
          - "/etc/passwd"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* 输出

```json
TODO
```
