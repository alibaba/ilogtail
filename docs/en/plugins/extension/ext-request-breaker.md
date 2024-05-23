# RequestBreaker: Request Throttling Extension

## Introduction

The `ext_request_breaker` extension, which implements the [extensions.RequestInterceptor](https://github.com/alibaba/ilogtail/blob/main/pkg/pipeline/extensions/request_interceptor.go) interface, is referenced in the `http_flusher` plugin, providing request rate limiting capabilities.

## Version

[Alpha](../stability-level.md)

## Configuration Parameters

| Parameter          | Type    | Required | Description                                                                                     |
|-------------------|-------|------|-------------------------------------------------------------------------------------------------|
| FailureRatio      | Float  | Yes   | The threshold for request throttling, the failure rate of requests within the WindowInSeconds window, default is 0.10 |
| WindowInSeconds   | Int    | Yes   | The time window for counting successful and failed requests, default is 10 seconds                    |

## Example

Use the `metric_mock` input plugin to generate data and submit the collected results in a `custom_single` protocol and `json` format to `http://localhost:8086/write`.

Additionally, configure the throttling strategy in the `flusher` processing:

```yaml
enable: true
version: v2
inputs:
  - Type: metric_mock
    GroupMeta:
      meta1: meta1
    GroupTags:
      tag1: tag1
  - Type: metric_mock
    GroupMeta:
      meta2: meta2
    GroupTags:
      tag2: tag2
flushers:
  - Type: flusher_http
    RemoteURL: "http://localhost:8086/write"
    Convert:
      Protocol: custom_single
      Encoding: json
    RequestInterceptors: 
      - Type: ext_request_breaker
        FailureRatio: 0.1
        WindowInSeconds: 10
extensions:
  - Type: ext_groupinfo_filter
    Tags:
      tag1: tag1
```
