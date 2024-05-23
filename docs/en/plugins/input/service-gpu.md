# GPU Metrics

## Introduction

The `service_gpu_metric` input plugin can collect NVIDIA GPU-related metrics (this plugin may not function properly without NVIDIA drivers).

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
|----------|-------------------|-------------|
| CollectIntervalMs | int(1000) | Specifies the plugin type as `service_gpu_metric`.|

## Examples

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: service_gpu_metric
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
    "metric_type": "gpu",
    "device": "0",
    "gpu_power_usage": "7",
    "gpu_temperature": "36",
    "gpu_util": "0",
    "gpu_total_memory": "7611",
    "gpu_free_memory": "7611",
    "gpu_memory_util": "0",
    "gpu_used_memory": "0",
    "__time__": "1663034534"
}
```

## Collected Metrics

| Name                | Description                           |
|-------------------|--------------------------------------|
| gpu_power_usage | Power usage of the GPU and related circuits (in milliwatts). |
| gpu_temperature | Temperature of the GPU (in Celsius). |
| gpu_util | GPU utilization rate. |
| gpu_memory_util | GPU memory usage rate. |
| gpu_used_memory | Used GPU memory (MB). |
| gpu_total_memory | Total GPU memory (MB). |
| gpu_free_memory | Available GPU memory (MB). |
