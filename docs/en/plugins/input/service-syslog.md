# Syslog Data

## Introduction

The `service_syslog` plugin listens for log data on specified addresses and ports using the Logtail plugin, then collects logs such as system logs from rsyslog, access or error logs from Nginx, and logs forwarded by syslog clients. [Source code](https://github.com/alibaba/ilogtail/blob/main/plugins/input/syslog/syslog.go)

## Version

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, `service_syslog` (required) | Plugin type, always set to `service_syslog`. |
| Address | String, `tcp://127.0.0.1:9999` | The address and port where Logtail listens for logs, with the format `tcp/udp://[ip]:[port]`. Ensure that the listening protocol, address, and port match the forwarding rules in the rsyslog configuration file. If the Logtail server has multiple IP addresses for receiving logs, you can set the address to `0.0.0.0` to listen on all server IPs. |
| MaxConnections | Integer, `100` | Maximum number of connections, only applicable for TCP. |
| TimeoutSeconds | Integer, `0` | Inactivity seconds before closing a remote connection. |
| MaxMessageSize | Integer, `64 * 1024` | Maximum byte size of messages received through the transport protocol. |
| KeepAliveSeconds | Integer, `300` | Connection keep-alive seconds, only for TCP. |
| ParseProtocol | String, `""` | Protocol to parse logs, defaults to empty (no parsing). Values: `rfc3164`, `rfc5424`, or `auto` for automatic detection. |
| IgnoreParseFailure | Boolean, `true` | Whether to ignore parsing failures. If not configured, it defaults to discarding logs on failure. Set to `false` to discard logs on failure. |
| AddHostname | Boolean, `false` | When listening on `/dev/log` with the `unixgram` protocol, the hostname field is not included, so setting `AddHostname` to `true` adds the current hostname to the parser, allowing it to parse the tag, program, and content fields correctly. |

## Example

This example uses UDP to listen on port 9009.

* Collector Configuration

```yaml
enable: true
inputs:
  - Type: service_syslog
    Address: udp://0.0.0.0:9009
flushers:
  - Type: flusher_stdout
    OnlyStdout: true  
```

* Output

```json
{
    "_program_": "",
    "_facility_": "-1",
    "_hostname_": "iZb***4prZ",
    "_content_": "<78>Dec 23 15:35:01 iZb***4prZ CRON[3364]: (root) CMD (command -v ***)",
    "_ip_": "172.**.**.5",
    "_priority_": "-1",
    "_severity_": "-1",
    "_unixtimestamp_": "1640244901606136313",
    "_client_ip_": "120.**.2**.90",
    "__time__": "1640244901"
}
```

* Field Interpretation

|Field|Description|
|---|---|
|`_program_`|Tag field from the protocol.|
| `_hostname_` | Hostname, if not provided in the log, it will be the current hostname. |
| `_program_` | Tag field from the protocol. |
| `_priority_` | Priority field from the protocol. |
| `_facility_` | Facility field from the protocol. |
| `_severity_` | Severity field from the protocol. |
| `_unixtimestamp_` | Timestamp of the log entry. |
| `_content_` | Log content; if parsing fails, this field contains the unparsed log. |
| `_ip_` | IP address of the current host. |
|`_client_ip_`|IP address of the client sending the log.|
