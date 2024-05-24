# GroupMetadata Aggregation

## Introduction

The `aggregator_metadata_group` plugin for the `aggregator` allows for re-aggregation of PipelineGroupEvents based on specific Metadata keys. It is designed for v2 versions only.

## Version

[Alpha](../stability-level.md)

## Configuration Parameters

| Parameter                  | Type       | Required | Description                                                                                     |
|---------------------|----------|------|--------------------------------------------------------------|
| Type                | String   | Yes   | Plugin type, set to `aggregator_metadata_group`.                         |
| GroupMetadataKeys   | []String | No    | A list of keys to group by their values; an empty list discards Metadata and only performs bundling aggregation. |
| GroupMaxEventLength | int      | No    | Maximum number of Events in a single PipelineGroupEvent during aggregation, default is 1024. |
| GroupMaxByteLength  | int      | No    | Maximum byte length of Events in a single PipelineGroupEvent, only supports ByteArray type, default is 3MiB. |
| DropOversizeEvent   | int      | No    | Whether to discard events exceeding the byte length limit; default is true.                   |

## Example

Collect raw byte streams from the `service_http_server` input plugin, using `GroupMetadataKeys` to define the fields for aggregation. The aggregated results will be sent via an HTTP request to a specified destination. In this example, the v2 plugin version is required.

In the example, first define the Metadata data source through the `QueryParams` in the input plugin configuration, then use the Metadata Key for aggregation in the aggregator plugin. Events with the same `db` value will be grouped together in a PipelineGroupEvent, which will be passed on. In the `flusher_http`, all Events within each PipelineGroupEvent will be sent in a single request.

* Input

```bash
curl --request POST 'http://127.0.0.1:12345?db=mydb' --data 'test,host=server01,region=cn value=0.1'
```

* Collector Configuration

```yaml
enable: true
version: "v2"
inputs:
  - Type: service_http_server
    Format: raw
    Address: "http://127.0.0.1:12345"
    QueryParams:
      - db
aggregators:
  - Type: aggregator_metadata_group
    GroupMetadataKeys:
      - db
flushers:
  - Type: flusher_http
    RemoteURL: "http://127.0.0.1:8086/write"
    Query:
      db: "%{metadata.db}"
    Convert:
      Protocol: raw
      Encoding: custom
```
