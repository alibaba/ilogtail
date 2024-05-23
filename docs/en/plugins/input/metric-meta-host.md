# Host Metadata

## Introduction

The `metric_meta_host` `input` plugin is used to collect metadata about a host, such as CPU type, memory size, processes, etc.

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter          | Type      | Required | Description                                                                                     |
| ----------- | ------- | ---- | --------------------------------------------------------------------------------------------- |
| Type        | String  | Yes   | Plugin type                                                                                     |
| CPU     | Boolean  | No    | Enables host CPU collection. Default: true.                                                     |
| Memory | Boolean  | No   | Enables host Memory collection. Default: true.                                                   |
| Net    | Boolean | No    | Enables host Net collection. Default: false.                                                      |
| Disk    | Boolean | No    | Enables host Disk collection. Default: false.                                                      |
| Process  | Boolean | No    | Enables host Process collection. Default: false.                                                  |
| ProcessNamesRegex  | String array | No    | Regular expression filter for process names.                                                  |
| Labels  | String map | No    | Custom labels.                                                                                   |
| ProcessIntervalRound  | Integer | No    | Process collection interval. Default: 5s.                                                       |

## Example

Collects host CPU, Memory, Net, Disk, and Process metadata information.

## Configuration Example

```yaml
enable: true
inputs:
  - Type: metric_meta_host
    CPU: true
    Memory: true
    Net: true
    Disk: true
    Process: true
    ProcessIntervalRound: 60
    ProcessNamesRegex:
      - ilogtail
    Labels:
      cluster: ilogtail-test-cluster
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

## Output

* Host metadata information

```json

{
    "__time__": 1658364854,
    "type": "HOST",
    "labels": "{\"platform_family\":\"\",\"ip\":\"172.16.2.0\",\"os\":\"linux\",\"virtualization_role\":\"guest\",\"boot_time\":\"1640524878\",\"platform_version\":\"3\",\"cluster\":\"ilogtail-test-cluster\",\"hostname\":\"iZj6cilksm3spb8xfv2t888\",\"platform\":\"alibaba\",\"kernel_version\":\"5.10.60-9.al8.x86_64\",\"kernel_arch\":\"x86_64\",\"virtualization_system\":\"\",\"host_id\":\"591c4a11-6b13-4735-a4ee-c46d11bf164e\"}",
    "attributes": {
        "MEM": {
            "mem_total": 16221536256,
            "swap_total": 0,
            "vsz_total": 35184372087808
        },
        "CPU": {
            "cache_size": 49152,
            "core_count": 2,
            "family": "6",
            "mhz": 2699.998,
            "model": "106",
            "model_name": "Intel(R) Xeon(R) Platinum 8369B CPU @ 2.70GHz",
            "processor_count": 4,
            "vendor_id": "GenuineIntel"
        },
        "NET": [
            {
                "addrs": [
                    {"addr": "127.0.0.1/8"},
                    {"addr": "::1/128"}
                ],
                "flags": ["up", "loopback"],
                "hardware_address": "",
                "index": 1,
                "mtu": 65536,
                "name": "lo"
            },
            {
                "addrs": [
                    {"addr": "172.16.2.0/24"},
                    {"addr": "fe80::216:3eff:fe01:4786/64"}
                ],
                "flags": ["up", "broadcast", "multicast"],
                "hardware_address": "00:16:3e:02:47:86",
                "index": 2,
                "mtu": 1500,
                "name": "eth0"
            },
            {
                "addrs": [
                    {"addr": "172.17.0.1/16"},
                    {"addr": "fe80::42:cfff:fe44:79dd/64"}
                ],
                "flags": ["up", "broadcast", "multicast"],
                "hardware_address": "02:42:cc:41:79:dd",
                "index": 3,
                "mtu": 1500,
                "name": "docker0"
            }
        ],
        "DISK": [
            {
                "device": "/dev/vda1",
                "fstype": "ext4",
                "mount_point": "/",
                "opts": "rw,relatime"
            },
            {
                "device": "/dev/vdb1",
                "fstype": "ext4",
                "mount_point": "/mnt",
                "opts": "rw,relatime"
            }
        ]
    },
    "id": "591c4a11-6b13-4735-a4ee-c46d11bf164e_172.16.2.0",
    "parents": "[]"
}
```

* Process metadata information

```json
{
    "__time__": 1658364348,
    "type": "PROCESS",
    "labels": "{\"cluster\":\"ilogtail-test-cluster\",\"hostname\":\"iZj6cilksm3spb8xfv2t888\",\"ip\":\"172.16.2.0\"}",
    "attributes": {
        "command": "./ilogtail",
        "exe": "/mnt/bin/ilogtail-1.1.0/ilogtail",
        "name": "ilogtail",
        "ppid": 3403955,
        "pid": 3404557
    },
    "id": "iZj6cilksm3spb8xfv2t888_172.16.2.0_PROCESS_3404557_1783946933",
    "parents": "[\"HOST:591c4a11-6b13-4735-a4ee-c46d11bf164e_172.16.2.0:iZj6cilksm3spb8xfv2t888\"]"
}
```
