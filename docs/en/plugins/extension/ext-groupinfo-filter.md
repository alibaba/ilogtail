# GroupInfoFilter FlushInterceptor Extension

## Introduction

The `ext_groupinfo_filter` extension plugin implements the [extensions.FlushInterceptor](https://github.com/alibaba/ilogtail/blob/main/pkg/pipeline/extensions/flush_interceptor.go) interface, allowing you to filter data before sending it to the remote destination in the `http_flusher` plugin. This provides the capability to selectively submit data based on certain criteria.

## Version

[Alpha](../stability-level.md)

## Configuration Parameters

| Parameter    | Type                    | Required | Description                                                  |
|-------------|-----------------------|----------|--------------------------------------------------------------|
| Tags         | Map<String,Condition> | No       | Tags in GroupInfo to filter, with key-value pairs             |
| Metas        | Map<String,Condition> | No       | Metadata in GroupInfo to filter, with key-value pairs          |

The `Condition` fields have the following structure:

| Field      | Type      | Required | Description                                                  |
|---------|---------|------|---------------------------|
| Pattern | String  | Yes    | Regular expression pattern for matching                     |
| Reverse | Boolean | No    | Reverse selection (exclude data that matches the pattern) |

## Example

Use the `metric_mock` input plugin to generate data and submit the collected results in `custom_single` protocol and `json` format to `http://localhost:8086/write`.

Filter the data in the flusher process to only send GroupInfo with a `tag1=tag1` in the Tags.

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
    FlushInterceptor:
      Type: ext_groupinfo_filter
extensions:
  - Type: ext_groupinfo_filter
    Tags:
      tag1:
        Pattern: tag1
```

## Using Named Extensions

When using multiple extensions of the same type within a single pipeline, you can assign a name to each instance. To reference a specific instance, use the full name in the flusher configuration.

For example, by changing `Type: ext_groupinfo_filter` to `Type: ext_groupinfo_filter/tag1` and adding the name `tag1`, you can reference the `ext_groupinfo_filter` instance with `tag1` in the `http_flusher` plugin.

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
    RemoteURL: "http://localhost:8086/write_select_tag1"
    Convert:
      Protocol: custom_single
      Encoding: json
    FlushInterceptor:
      Type: ext_groupinfo_filter/tag1
  - Type: flusher_http
    RemoteURL: "http://localhost:8086/write_select_tag2"
    Convert:
      Protocol: custom_single
      Encoding: json
    FlushInterceptor:
      Type: ext_groupinfo_filter/tag2
extensions:
  - Type: ext_groupinfo_filter/tag1
    Tags:
      tag1:
        Pattern: tag1
  - Type: ext_groupinfo_filter/tag2
    Tags:
      tag2:
        Pattern: tag2
```
