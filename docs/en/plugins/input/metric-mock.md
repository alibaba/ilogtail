# Mock Data - Metric

## Introduction

The `metric_mock` plugin is a plugin designed to simulate Metric-type input data by adjusting parameters to generate different mock inputs. [Source code](https://github.com/alibaba/ilogtail/blob/main/plugins/input/mock/input_mock.go)

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, Required, no default (Mandatory) | Plugin type, always set to `metric_mock`. |
| Tags | Map, Key and Value are Strings, `{}` | Can add tags as needed to the mock data. |
| Fields | Map, Key and Value are Strings, `{}` | Can add fields as needed to the mock data. |
| Index | Long, `0` | Starting index for generated mock data (starts from the next index). |
| OpenPrometheusPattern | Boolean, `false` | Whether to generate mock data in Prometheus format. |

## Examples

* Configuration 1

```yaml
enable: true
inputs:
  - Type: metric_mock
    Fields:
      field1: field1
      field2: field2
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output 1

```json
{
  "field2": "field2",
  "Index": "1",
  "field1": "field1",
  "__time__": "1658807050"
}
{
  "field1": "field1",
  "field2": "field2",
  "Index": "2",
  "__time__": "1658807051"
}
```

* Configuration 2

```yaml
enable: true
inputs:
  - Type: metric_mock
    Index: 100
    Tags:
      tag1: tag1
      tag2: tag2
    Fields:
      field1: field1
      field2: field2
    OpenPrometheusPattern: true
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output 2

```json
{
  "__name__": "metrics_mock",
  "__labels__": "field1#$#field1|field2#$#field2|tag1#$#tag1|tag2#$#tag2",
  "__time_nano__": "1658806869597190887",
  "__value__": "101",
  "__time__": "1658806869"
}
{
  "__name__": "metrics_mock",
  "__labels__": "field1#$#field1|field2#$#field2|tag1#$#tag1|tag2#$#tag2",
  "__time_nano__": "1658806870597391426",
  "__value__": "102",
  "__time__": "1658806870"
}
```
