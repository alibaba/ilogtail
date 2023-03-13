# 添加字段

## 简介

`processor_cloudmeta`插件可以添加为日志增加云平台元数据信息。

## 配置参数

| 参数          | 类型                | 是否必选 | 说明                                                                                                                                                    |
|-------------|-------------------|------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| Platform    | String            | 是    | 云平台名称，目前支持 alibaba_cloud、auto，auto 模式支持动态选择云平台。                                                                                                       |
| AddMetas    | []string          | 是    | 增加元信息配置,，默认追加名字为元信息标签名，支持标签请参考支持元信息标签。                                                                                                                |
| JSONPath    | String            | 否    | 为空时直接添加字段，不为空时表示为json序列化字段增加元数据标签，支持多层结构增加云平台元信息，最内层结构需要为json结构，如存在 Log_Content 结构`a: {"b":{}}`，当在 a 子结构下追加是JSONPath为`a`，当在 b 子结构下追加时JSONPath 为 `a.b` |
| RenameMetas | map[string]string | 否    | 重命名Meta名称                                                                                                                                             |
| ReadOnce    | bool              | 否    | true表示仅读取一次，不支持感知动态变化，默认值false。                                                                                                                       |

## 支持元信息标签

| 标签                        | 说明                                                                                       |
|---------------------------|------------------------------------------------------------------------------------------|
| `__cloud_instance_id__`   | 云服务器实例id                                                                                 |
| `__cloud_instance_name__` | 云服务器实例名                                                                                  |
| `__cloud_zone__`          | 云服务器实例zone                                                                               |
| `__cloud_region__`        | 云服务器实例region                                                                             |
| `__cloud_instance_type__` | 云服务器实例规格                                                                                 |
| `__cloud_vswitch_id__`    | 云服务器实例vswitch id                                                                         |
| `__cloud_vpc_id__`        | 云服务器实例vpcid                                                                              |
| `__cloud_image_id__`      | 云服务器实例镜像id                                                                               |
| `__cloud_max_ingress__`   | 云服务器实例最大内网入带宽                                                                            |
| `__cloud_max_egress__`    | 云服务器实例最大内网出带宽                                                                            |
| `__cloud_instance_tags__` | 云服务器实例标签前缀，如配置 `cloud_instance_tags：custom_tag` ，当云实例存在标签`a：b` 时，将增加 `custom_tag_a:b` 数据 |

## json Mode 配置举例

| Data Input         | JSONPath  | AddMetas           | RenameMetas               | Data Output                         |
|--------------------|-----------|--------------------|---------------------------|-------------------------------------|
| `A:{}`             | ""        | ["__cloud_zone__"] | {}                        | `A:{},"__cloud_zone__":"xxxx"`      |
| `B:{}`             | "A"       | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | `B:{},A:{"zone":"xxxx"}`            |
| `A:""`             | "A"       | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | 选中A字段，但A字段非json结构，因此跳过追加，输出`A:""`   |
| `A:{"a":{"b":{}}}` | "A.a.b"   | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | `A:{"a":{"b":{"zone":"xxxx"}}}`     |
| `A:{"a":{"b":{}}}` | "A.a.b.c" | ["__cloud_zone__"] | {"__cloud_zone__":"zone"} | `A:{"a":{"b":{c:{"zone":"xxxx"}}}}` |

## 样例

### content 模式

* 采集配置

```yaml
enable: true
inputs:
  - Type: service_mock
    LogsPerSecond: 10
    MaxLogCount: 100
    Fields:
      content: "abc"
processors:
  - Type: processor_cloudmeta
    Platform: mock
    Mode: add_fields
    AddMetas:
      __cloud_instance_id__: instance_id_name
        __cloud_instance_tags__: instance_tag_prefix
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```text
2023-03-09 17:11: 17 {"content": "abc", "Index":"1", "instance_id_name": "id_xxx", "instance_tag_prefix_tag_key": "tag_val","__time__": "1678353077"
}
2023-03-09 17: 11: 17 {"content": "abc", "Index": "2", "instance_id_name": "id_xxx", "instance_tag_prefix_tag_key": "tag_val", "__time__": "1678353077"}
2023-03-09 17: 11: 17 {
"Index":"3", "content": "abc", "instance_id_name": "id_xxx","instance_tag_prefix_tag_key": "tag_val", "__time__": "1678353077"
}
2023-03-09 17: 11: 17 {"content": "abc", "Index": "4", "instance_id_name": "id_xxx", "instance_tag_prefix_tag_key": "tag_val", "__time__": "1678353077"
}
2023-03-09 17: 11:17 {"Index": "5", "content": "abc","instance_id_name": "id_xxx", "instance_tag_prefix_tag_key": "tag_val", "__time__":"1678353077"
}
2023-03-09 17: 11: 17 {
"content": "abc", "Index": "6", "instance_id_name": "id_xxx", "instance_tag_prefix_tag_key": "tag_val", "__time__": "1678353077"
}

``` 

### json 模式

* 采集配置

```yaml
enable: true
inputs:
  - Type: service_mock
    LogsPerSecond: 10
    MaxLogCount: 100
    Fields:
      content: "{\"a\":{\"b\":{}}}"
processors:
  - Type: processor_cloudmeta
    Platform: mock
    Mode: modify_json
    JSONPath: "content.a.b"
    AddMetas:
      "__cloud_instance_id__": instance_id_name
        __cloud_instance_tags__: instance_tag_prefix
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```text
2023-03-09 17:14:31 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"1","__time__":"1678353271"}
2023-03-09 17:14:31 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"2","__time__":"1678353271"}
2023-03-09 17:14:31 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"3","__time__":"1678353271"}
2023-03-09 17:14:31 {"content":"{\"a\":{\"b\":{\"instance_id_name\":\"id_xxx\",\"instance_tag_prefix_tag_key\":\"tag_val\"}}}","Index":"4","__time__":"1678353271"}
```
