# MetricInput Example Plugin

## Introduction

The `metric_input_example` serves as a reference example for developing a `MetricInput` class plugin. It periodically generates simulated metric data. [Source code](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/metric_example.go)

## Stability

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, Required (no default) | Plugin type, always set to `metric_input_example`. |

## Example

### Collection Configuration

```yaml
enable: true
inputs:
  - Type: metric_input_example
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

### Output

```json
{
    "__name__": "example_counter",
    "__labels__": "hostname#$#$************************|ip#$#$172.***.***.***",
    "__time_nano__": "1658491078378371632",
    "__value__": "101",
    "__time__": "1658491078"
}
```
