# 添加字段

## 简介

`processor_cloudmeta`插件可以添加为日志增加云平台元数据信息。

## 配置参数

| 参数       | 类型                | 是否必选 | 说明                                                                                                                 |
|----------|-------------------|------|--------------------------------------------------------------------------------------------------------------------|
| Platform | String            | 是    | 云平台名称，目前支持aliyun。                                                                                                  |
| Mode     | String            | 否    | 支持content与json 两种模式，content 模式为protocol.Log 协议增加Log_Content字段，json模式为具体某个为json序列化的Log_Content增加元数据标签。默认为content模式。 |
| JSONKey  | String            | 否    | Mode为json模式生效，用来选取具体某个增加云平台元信息的Log_Content结构。                                                                      |
| JSONPath | String            | 否    | Mode为json模式生效，支持多层结构增加云平台元信息，最内层结构需要为json结构                                                                        |
| AddMetas | map[string]string | 否    | 增加元信息配置。                                                                                                           |

## 支持元信息标签

| 标签                      | 说明                                                                                       |
|-------------------------|------------------------------------------------------------------------------------------|
| __cloud_instance_id__   | 云服务器实例id                                                                                 |
| __cloud_instance_name__ | 云服务器实例名                                                                                  |
| __cloud_zone__          | 云服务器实例zone                                                                               |
| __cloud_region__        | 云服务器实例region                                                                             |
| __cloud_instance_type__ | 云服务器实例规格                                                                                 |
| __cloud_vswitch_id__    | 云服务器实例vswitch id                                                                         |
| __cloud_vpc_id__        | 云服务器实例vpcid                                                                              |
| __cloud_image_id__      | 云服务器实例镜像id                                                                               |
| __cloud_max_ingress__   | 云服务器实例最大内网入带宽                                                                            |
| __cloud_max_egress__    | 云服务器实例最大内网出带宽                                                                            |
| __cloud_instance_tags__ | 云服务器实例标签前缀，如配置 `cloud_instance_tags：custom_tag` ，当云实例存在标签`a：b` 时，将增加 `custom_tag_a:b` 数据 |

## json Mode 配置举例

| Data Input         | JSONPath | AddMetas              | JSONKey | Data Output                                                                     |
|--------------------|----------|-----------------------|---------|---------------------------------------------------------------------------------|
| `A:{}`             | ""       | {"cloud_zone":"zone"} | A       | 选中A字段，并且JSONPath为空数目第一层结构，且结构为json合法，因此输出数据为 `A:{"zone":"real zone data"}`      |
| `B:{}`             | ""       | {"cloud_zone":"zone"} | A       | 选中A字段，但无具体字段，因此跳过追加，输出`B:{}`                                                    |
| `A:""`             | ""       | {"cloud_zone":"zone"} | A       | 选中A字段，但A字段非json结构，因此跳过追加，输出`A:""`                                               |
| `A:{"a":{"b":{}}}` | "a.b"    | {"cloud_zone":"zone"} | A       | 选中A字段，且JSONPath a.b 下为合法json 结构，因此输出数据`A:{"a":{"b":{"zone":"real zone data"}}}` |
| `A:{"a":{"b":{}}}` | "a.b.c"  | {"cloud_zone":"zone"} | A       | 选中A字段，但JSONPath a.b.c 下无合法json 结构，因此输出数据`A:{"a":{"b":{}}}`                      |

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
    Mode: content
    AddMetas:
      __cloud_instance_id__: instance_id_name
      __cloud_instance_tags__: instance_tag_prefix
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* 输出

```json
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
    Mode: content
    JSONKey: content
    JSONPath: "a.b"
    AddMetas:
      __cloud_instance_id__: instance_id_name
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
