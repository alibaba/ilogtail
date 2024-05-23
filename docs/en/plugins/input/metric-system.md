# Host Monitoring Data

## Introduction

The `metric_system_v2` `input` plugin is used to collect host monitoring data, such as CPU, memory, load, disk, and network metrics.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter          | Type      | Required | Description                                                                                     |
| ----------- | ------- | ---- | --------------------------------------------------------------------------------------------- |
| Type        | String  | Yes   | Plugin type                                                                                     |
| CPU         | Boolean  | No    | Whether to enable CPU metric collection. <br>Default: true.                                   |
| Mem         | Boolean  | No    | Whether to enable Memory metric collection. <br>Default: true.                                  |
| Disk        | Boolean | No    | Whether to enable Disk metric collection. <br>Default: true.                                   |
| Net         | Boolean | No    | Whether to enable Network metric collection. <br>Default: true.                                  |
| Protocol    | Boolean | No    | Whether to enable Protocol metric collection. <br>Default: true.                                  |
| TCP         | Boolean | No    | Whether to enable TCP-specific Network collection. <br>Default: false.                             |
| OpenFd      | Boolean | No    | Whether to enable Open File Descriptors metric collection. <br>Default: true.                     |
| CPUPercent  | Boolean | No    | Whether to enable CPU usage percentage metric collection. <br>Default: true.                     |
| Labels      | String Map | No    | Custom labels.                                                                                   |

## Example

Collect host metrics like CPU, Memory, Net, Disk, and Process data.

## Configuration Example

```yaml
enable: true
inputs:
  - Type: metric_system_v2
    CPU: true
    Mem: true
    Disk: true
    Net: true
    Protocol: true
    TCP: true
    OpenFd: true
    CPUPercent: true
    Labels:
      cluster: ilogtail-test-cluster
flushers:
  - Type: flusher_stdout
    FileName: /tmp/124.log
    OnlyStdout: false
```

## Output

* Host metric information

```json
{
    "__name__": "net_out_pkt",
    "__labels__": "cluster#$#ilogtail-test-cluster|hostname#$#master-1-1.c-ca9717110efa1b40|hostname#$#test-1|interface#$#eth0|ip#$#10.1.37.31",
    "__time_nano__": "1680079323040664058",
    "__value__": "32.764761658490045",
    "__time__": "1680079323"
}
```
