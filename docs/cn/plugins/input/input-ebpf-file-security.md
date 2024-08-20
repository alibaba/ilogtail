# input_ebpf_file_security 插件

## 简介

`input_ebpf_file_security`插件可以实现利用ebpf探针采集文件安全相关动作。

## 版本

[Dev](../stability-level.md)

## 配置参数

|  **参数**  |  **类型**  |  **是否必填**  |  **默认值**  |  **说明**  |
| --- | --- | --- | --- | --- |
|  Type  |  string  |  是  |  /  |  插件类型。固定为input\_ebpf\_file\_security  |
|  ProbeConfig  |  \[object\]  |  否  |  /  |  插件配置参数列表  |
|  ProbeConfig.CallNameFilter  |  \[string\]  |  否  |  该插件支持的所有 callname  |  内核挂载点过滤器，按照白名单模式运行，不填表示配置该插件所支持的所有挂载点  |
|  ProbeConfig.FilePathFilter  |  \[string\]  |  否  |  空  |  文件路径过滤器，遵循前缀匹配的原则，不填表示不进行过滤  |

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
      - CallNameFilter: 
        - "security_file_permission"
        FilePathFilter: 
          - "/etc/passwd"
          - "/lib"
      - CallNameFilter: 
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
