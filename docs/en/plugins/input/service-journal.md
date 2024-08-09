# Journal Data

## Introduction

The `service_journal` plugin supports collecting Linux system logs from binary files using systemd, a system and service manager designed specifically for Linux. As the primary process (PID=1), systemd initializes the system, starts and maintains various user space services. It consolidates log management for all Units, typically found in `/etc/systemd/journald.conf`. [Source Code](https://github.com/alibaba/ilogtail/blob/main/plugins/input/journal/input_journal.go)

## Version

[Stable](../stability-level.md)

## Features

* Supports setting an initial sync position, allowing for automatic checkpoint saving after the first collection, without worrying about application restarts.
* Supports filtering specific Units.
* Supports collecting kernel logs.
* Supports automatic parsing of log levels.
* Supports collecting journal logs in container mode, ideal for Docker and Kubernetes scenarios.

## Limitations

* Logtail requires a version of 0.16.18 or later.
* The operating system must support the systemd journal log format.

## Use Cases

1. Monitor kernel events, automatically triggering alerts when exceptions occur.
2. Collect all system logs for long-term storage, reducing disk space usage.
3. Gather software (Unit) output logs for analysis or alerts.
4. Collect all journal logs for quick keyword searches or log retrieval, significantly faster than Journalctl queries.

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type    | String, Required (fixed as `service_journal`) | Plugin type. |
| JournalPaths | Array of Strings, Required (no default) | Journal log paths, usually the folder where the journal logs are located, e.g., `/var/log/journal`. |
| SeekPosition | String, `tail` | Initial collection method, `head` for all data, `tail` for new data after configuration. |
| Kernel | Boolean, `true` | Set to `false` to exclude kernel logs. |
| Units | Array of Strings, `[]` | List of Units to collect, empty for all. |
| ParseSyslogFacility | Boolean, `false` | Whether to parse the syslog facility field. |
| ParsePriority | Boolean, `false` | Whether to parse the Priority field. |
| UseJournalEventTime | Boolean, `false` | Use the journal log's timestamp field as the log time, i.e., use the collection time as the log time (real-time log collection is usually within 3 seconds). |
| CursorFlushPeriodMs | Integer, `5000` | Time interval for flushing log read checkpoints. |
| CursorSeekFallback | String, `SeekPositionTail` | Position to fallback to for log read checkpoints. |
| Identifiers | Array of Strings, `[]` | syslog identifiers, can be added to monitors. |
| MatchPatterns | Array of Strings, `[]` | Matching rules, can be added to monitors. |

**ParsePriority** mapping table:

```go
"0": "emergency"
"1": "alert"
"2": "critical"
"3": "error"
"4": "warning"
"5": "notice"
"6": "informational"
"7": "debug"
```

## Examples

### Example 1

Collect journal logs from the default `/var/log/journal` path.

* Configuration 1

```yaml
enable: true
inputs:
  - Type: service_journal
    JournalPaths:
      - "/var/log/journal"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output 1

```json
{
    "PRIORITY": "6",
    "_GID": "0",
    "_TRANSPORT": "syslog",
    "_SYSTEMD_OWNER_UID": "0",
    "_BOOT_ID": "bab**************54b",
    "_PID": "21848",
    "_MACHINE_ID": "************************",
    "_CAP_EFFECTIVE": "3fffffffff",
    "_COMM": "crond",
    "_HOSTNAME": "iZj*****************1hZ",
    "_SYSTEMD_SLICE": "user-0.slice",
    "MESSAGE": "(root) CMD (/usr/lib64/sa/sa1 1 1)",
    "_CMDLINE": "/usr/sbin/CROND -n",
    "SYSLOG_FACILITY": "9",
    "SYSLOG_IDENTIFIER": "CROND",
    "_AUDIT_LOGINUID": "0",
    "_SYSTEMD_SESSION": "3319",
    "_UID": "0",
    "_EXE": "/usr/sbin/crond",
    "SYSLOG_PID": "21848",
    "_AUDIT_SESSION": "3319",
    "_SYSTEMD_CGROUP": "/user.slice/user-0.slice/session-3319.scope",
    "_SYSTEMD_UNIT": "session-3319.scope",
    "_SOURCE_REALTIME_TIMESTAMP": "1658823001526225",
    "_realtime_timestamp_": "1658823001526482",
    "_monotonic_timestamp_": "1637927744052",
    "__time__": "1658823031"
}
```

### Example 2

In a Kubernetes scenario, collect host system logs using Logtail's DaemonSet mode. Since the logs contain many unimportant fields, use a processing plugin to select only the more important log fields.

* Configuration 2

```yaml
enable: true
inputs:
  - Type: service_journal
    ParsePriority: true
    ParseSyslogFacility: true
    JournalPaths:
      - "/logtail_host/var/log/journal"
processors:
  - Type: processor_filter_regex
    Exclude:
      UNIT: "^libcontainer.*test"
  - Type: processor_pick_key
    Include:
      - MESSAGE
      - PRIORITY
      - _EXE
      - _PID
      - _SYSTEMD_UNIT
      - _realtime_timestamp_
      - _HOSTNAME
      - UNIT
      - SYSLOG_FACILITY
      - SYSLOG_IDENTIFIER
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output 2

```json
{
    "MESSAGE": "ejected connection from \"192.168.0.251:48914\" (error \"EOF\", ServerName \"\")",
    "PRIORITY": "informational",
    "SYSLOG_IDENTIFIER": "etcd",
    "_EXE": "/opt/etcd-v3.3.8/etcd",
    "_HOSTNAME": "iZb*****************ueZ",
    "_PID": "10590",
    "_SYSTEMD_UNIT": "etcd.service",
    "__source__": "***.***.***.***",
    "__tag__:__hostname__": "logtail-ds-dp48x",
    "_realtime_timestamp_": "1547975837008708",
}
```
