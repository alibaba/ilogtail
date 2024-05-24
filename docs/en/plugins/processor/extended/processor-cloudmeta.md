# Adding Cloud Asset Metadata

## Introduction

The `processor_cloud_meta` plugin can be configured to add cloud platform metadata to log entries.

## Version

[Alpha](../stability-level.md)

## Configuration Parameters

| Parameter         | Type        | Required | Description                                                                                   |
|-------------------|-------------|----------|-------------------------------------------------------------------------------------------------|
| Metadata          | []string    | Yes      | Configuration to add metadata, default adds the tag name as metadata. Supported tags are listed below. |
| Platform          | String      | No       | Cloud platform name, currently supports `alibaba_cloud_ecs`, `auto`, with `auto` mode dynamically selecting the platform. Default is `auto`. |
| JSONPath          | String      | No       | Empty for direct addition, non-empty for JSON-structured fields to add cloud platform metadata. Supports multi-level structures. |
| RenameMetadata     | map[string]string | No      | Mapping to rename metadata keys. |
| ReadOnce          | bool        | No       | True to read once only, does not support dynamic changes. Default is false. |

## Supported Metadata Tags

| Tag                | Description                                                                                     |
|-------------------|-------------------------------------------------------------------------------------------------|
| `__cloud_instance_id__` | Cloud server instance ID                                                                       |
| `__cloud_instance_name__` | Cloud server instance name                                                                       |
| `__cloud_zone__`    | Cloud server instance zone                                                                       |
| `__cloud_region__`   | Cloud server instance region                                                                      |
| `__cloud_instance_type__` | Cloud server instance type                                                                       |
| `__cloud_vswitch_id__` | Cloud server instance VSwitch ID                                                                  |
| `__cloud_vpc_id__`    | Cloud server instance VPC ID                                                                     |
| `__cloud_image_id__`  | Cloud server instance image ID                                                                    |
| `__cloud_max_ingress__` | Cloud server instance's maximum internal network ingress bandwidth. |
| `__cloud_max_egress__` | Cloud server instance's maximum internal network egress bandwidth. |
| `__cloud_instance_tags__` | Prefix for cloud instance tags, e.g., if configured as `cloud_instance_tags: custom_tag`, adds `custom_tag_{tag_key}` for tags like `a: b` |

## JSON Mode Configuration Examples

| Data Input         | JSONPath  | Metadata           | RenameMetadata            | Data Output                         |
|--------------------|-----------|--------------------|---------------------------|-------------------------------------|
| `A:{}`             | ""        | ["__cloud_zone__"] | {}                        | `A:{},"__cloud_zone__":"xxxx"`      |
| `B:{}`             | "A"       | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | `B:{},A:{"zone":"xxxx"}`            |
| `A:""`             | "A"       | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | Skips A field since it's not a JSON structure, output is `A:""`   |
| `A:{"a":{"b":{}}}` | "A.a.b"   | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | `A:{"a":{"b":{"zone":"xxxx"}}}`     |
| `A:{"a":{"b":{}}}` | "A.a.b.c" | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | `A:{"a":{"b":{c:{"zone":"xxxx"}}}}` |

## Sample

## # Content Mode

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: service_mock
    LogsPerSecond: 10
    MaxLogCount: 100
    Fields:
      content: "abc"
processors:
  - Type: processor_cloud_meta
    Platform: mock
    Mode: add_fields
    Metadata:
      - "__cloud_instance_id__"
      - "__cloud_instance_tags__"
    RenameMetadata:
      __cloud_instance_id__: instance_id_name
      __cloud_instance_tags__: instance_tag_prefix
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```text
2023-03-13 20:42:04 {"content":"{\"a\":{\"b\":{}}}","Index":"1","instance_id_name":"id_xxx","instance_tag_prefix_tag_key":"tag_val","__time__":"1678711324"}

2023-03-13 20:42:04 {"content":"{\"a\":{\"b\":{}}}","Index":"2","instance_id_name":"id_xxx","instance_tag_prefix_tag_key":"tag_val","__time__":"1678711324"}

2023-03-13 20:42:04 {"content":"{\"a\":{\"b\":{}}}","Index":"3","instance_id_name":"id_xxx","instance_tag_prefix_tag_key":"tag_val","__time__":"1678711324"}

2023-03-13 20:42:04 {"content":"{\"a\":{\"b\":{}}}","Index":"4","instance_id_name":"id_xxx","instance_tag_prefix_tag_key":"tag_val","__time__":"1678711324"}

2023-03-13 20:42:04 {"content":"{\"a\":{\"b\":{}}}","Index":"5","instance_id_name":"id_xxx","instance_tag_prefix_tag_key":"tag_val","__time__":"1678711324"}

2023-03-13 20:42:04 {"content":"{\"a\":{\"b\":{}}}","Index":"6","instance_id_name":"id_xxx","instance_tag_prefix_tag_key":"tag_val","__time__":"1678711324"}
```

## # JSON Mode

* Collection Configuration

```yaml
enable: true
inputs:
  - Type: service_mock
    LogsPerSecond: 10
    MaxLogCount: 100
    Fields:
      content: "{\"a\":{\"b\":{}}}"
processors:
  - Type: processor_cloud_meta
    Platform: mock
    JSONPath: "content.a.b"
    Metadata:
      - "__cloud_instance_id__"
      - "__cloud_instance_tags__"
    RenameMetadata:
      __cloud_instance_id__: instance_id_name
      __cloud_instance_tags__: instance_tag_prefix
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```text
2023-03-13 20:39:02 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"1","__time__":"1678711142"}

2023-03-13 20:39:02 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"2","__time__":"1678711142"}

2023-03-13 20:39:02 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"3","__time__":"1678711142"}

2023-03-13 20:39:02 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"4","__time__":"1678711142"}

2023-03-13 20:39:02 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"5","__time__":"1678711142"}
```
