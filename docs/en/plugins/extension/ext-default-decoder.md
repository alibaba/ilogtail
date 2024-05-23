# DefaultDecoder Decoder Extension

## Introduction

The [ext_default_decoder](https://github.com/alibaba/ilogtail/blob/main/plugins/extension/default_decoder/default_decoder.go) extension implements the [Decoder](https://github.com/alibaba/ilogtail/blob/main/pkg/pipeline/extensions/decoder.go) interface, allowing it to be used in plugins like `service_http_server` and `udp_server` to parse data from various protocols.

## Version

[Alpha](../stability-level.md)

## Configuration Parameters

| Parameter                | Type      | Required | Description                                                                                           |
|-------------------|---------|------|--------------------------------------------------------------------------------------------------------|
| Format            | String  | Yes   | The specific protocol, [view the list of supported formats](https://github.com/alibaba/ilogtail/blob/master/pkg/protocol/decoder/common/comon.go) |
| FieldsExtend      | Boolean | No    | Enables enhanced field functionality, default is false, only effective when Format is set to influxdb. |
| DisableUncompress | Boolean | Yes   | Disables data decompression, default is false, only applicable when Format is set to raw.                |

## Example

To use the `service_http_server` input plugin, configure it to receive `influxdb` protocol data.

```yaml
enable: true
inputs:
  - Type: service_http_server
    Address: ":18888"
    Format: influxdb
    Decoder: ext_default_decoder
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

To test it, run the following command:

```shell
curl http://localhost:18888/write?db=mydb --data-binary 'weather,city=hz value=34'
```
