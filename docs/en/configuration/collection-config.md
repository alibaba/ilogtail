# Collection Configuration

The iLogtail pipeline is defined through collection configuration files, with one configuration file per pipeline.

## Format

Collection configuration files support both JSON and YAML formats. The top-level fields for each collection configuration are as follows:

| **Parameter**                           | **Type**     | **Required** | **Default** | **Description**                          |
|----------------------------------|------------|----------|---------|---------------------------------|
| enable                           | bool       | No        | true    | Whether to use the current configuration. |
| global                           | object     | No        | None     | Global configuration.                 |
| global.EnableTimestampNanosecond | bool       | No        | false   | Enable nanosecond timestamps for increased precision. |
| inputs                           | \[object\] | Yes       | /        | List of input plugins. Currently, only one input plugin is allowed. |
| processors                       | \[object\] | No        | None     | List of processing plugins.            |
| aggregators                      | \[object\] | No        | None     | List of aggregation plugins. Currently, only one aggregator is allowed, and all output plugins share it. |
| flushers                         | \[object\] | Yes       | /        | List of output plugins. At least one output plugin is required. |
| extensions                      | \[object\] | No        | None     | List of extension plugins.              |

In `inputs`, `processors`, `aggregators`, `flushers`, and `extensions`, you can include any number of [plugins](../plugins/overview.md).

## Organization

Local collection configuration files are stored by default in the `./config/local` directory, with one file per configuration, and the file name matching the configuration name.

## Hot Loading

Collection configuration files support hot reloading. iLogtail will automatically detect and reload configurations when you add or modify files in the `./config/local` directory. The default生效等待时间 is 10 seconds, which can be adjusted using the `config_scan_interval` startup parameter.

## Example

A typical collection configuration looks like this:

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/reg.log
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Regex: (\d*-\d*-\d*)\s(.*)
    Keys:
      - time
      - msg
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

For more common collection configurations, refer to the `example_config` directory.
