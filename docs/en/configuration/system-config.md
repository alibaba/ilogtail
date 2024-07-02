# System Parameters

When `iLogtail` starts, it loads the `ilogtail_config.json` configuration file, which specifies some basic configuration items for `iLogtail` to run smoothly.

## Parameter List

| Parameter                  | Type     | Description                                                                                                                                                                                                                     |
| ----------------------- | ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cpu_usage_limit`         | double | CPU usage threshold, expressed as a percentage of a single core. Range: 0.1 to the number of CPU cores on the machine. <br><strong>Caution:</strong> `cpu_usage_limit` is a soft limit, and `iLogtail` may exceed the limit, triggering a circuit breaker after 5 minutes. It will automatically restart. <br>For example, setting it to 0.4 means that the log service will limit `iLogtail's` CPU usage to 40% of a single core, and it will restart automatically if it exceeds the limit. |
| `mem_usage_limit`         | Int    | Memory usage threshold. <br><strong>Caution:</strong> `mem_usage_limit` is a soft limit, and `iLogtail` may exceed the limit, triggering a circuit breaker after 5 minutes, followed by automatic restart.                                                                                     |
| `default_access_key_id`   | String | The `access_id` to write to `SLS`, requiring write permissions.                                                                                                                                                                          |
| `default_access_key`      | String | The `access_key` to write to `SLS`, requiring write permissions.                                                                                                                                                                         |
| `config_scan_interval`    | Int    | Interval for scanning local configuration updates, in seconds.                                                                                                                  |
| `data_server_port`        | Int    | <p>The protocol used by `flusher_sls` to send data to `SLS`. </p><p>Values: 443 (default) for HTTPS, or 80 for HTTP. </p><p>If using an internal SLS domain, it's recommended to use HTTP for higher transmission performance.</p> |
| `send_running_status`      | Bool   | To better understand `iLogtail's` usage and inform targeted development planning, `iLogtail` will report anonymized running statistics. You can also manually disable this feature.                                                  |
| `host_path_blacklist`     | String | A global blacklist of host paths, with sub-string matching. In Linux, separate multiple substrings with a colon (:), and in Windows, use a semicolon (;). For example, to prevent logging from a NAS mount, configure it as `/volumes/kubernetes.io~csi/nas-`. |

## Sample Configuration

```json
{
    "default_access_key_id": "",
    "default_access_key": "",
    "cpu_usage_limit": 0.4,
    "mem_usage_limit": 384
}
```

## Environment Variables

### Containerd Runtime Configuration Variables

| Parameter                  | Type     | Description                                                                                                         |
| ----------------------- |--------|------------------------------------------------------------------------------------------------------------|
| `USE_CONTAINERD`          | Bool   | Whether to use the containerd runtime. `iLogtail` will automatically detect it.                                            |
| `CONTAINERD_SOCK_PATH`     | String | Custom containerd socket path. Optional. Default is `/run/containerd/containerd.sock`. You can find the custom value by examining the `grpc.address` field in `/etc/containerd/config.toml`. |
| `CONTAINERD_STATE_DIR`    | String | Custom containerd data directory. Optional. You can find the custom value by examining the `state` field in `/etc/containerd/config.toml`.                                      |
| `LOGTAIL_LOG_LEVEL` | String | Controls the log level for `/apsara/sls/ilogtail` and the Golang plugin, supporting generic log levels like trace, debug, info, warning, error, and fatal.|

> Since Kubernetes already has built-in resource limits, if you plan to deploy `iLogtail` in a Kubernetes environment, you can set `cpu_usage_limit` and `mem_usage_limit` to a large value (like 99999999) to effectively "disable" the `iLogtail` circuit breaker feature.
