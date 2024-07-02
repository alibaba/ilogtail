# processor_otel_trace Plugin

## Version

[Stable](../stability-level.md)

## Configuration Parameters

Here's the fixed table:

| Parameter                     | Type      | Required | Description                                                                                                                  |
|-------------------------------|---------|----------|------------------------------------------------------------------------------------------------------------------------------|
| SourceKey                     | String   | Yes      | Original field name.                                                                                                          |
| Format                        | String   | Yes      | Converted format (enumerated type). Three options: protobuf, json, protojson                                                               |
| NoKeyError                    | Boolean  | No       | Whether to throw an error if no corresponding field is found. Default is false.                                                 |
| TraceIDNeedDecode             | Boolean  | No       | Whether to Base64-encode TraceID. Default is false. When Format is protojson, consider setting this to true. Reason: protojson decodes TraceID during conversion, while ProtoJson format may not encode TraceID, causing failure. |
| SpanIDNeedDecode              | Boolean  | No       | Whether to Base64-encode SpanID. Default is false. Similar to TraceIDNeedDecode, adjust for protojson if needed.                  |
| ParentSpanIDNeedDecode        | Boolean  | No       | Whether to Base64-encode ParentSpanID. Default is false. Similar to the previous two, adjust for protojson if needed.             |

## Examples

Collect logs from the `/home/test-log` directory's `simple.log` file, extracting log information based on the provided configuration options.

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_otel_trace
    SourceKey: "content"
    Format: protojson
    TraceIDNeedDecode: true
    SpanIDNeedDecode: true
    ParentSpanIDNeedDecode: true
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Input

```bash
echo '{ "resource": {}, "scopeSpans": [ { "scope": { "name": "com.alibaba.cloud", "attributes": [] }, "spans": [ { "traceId": "31646461386336653337343330356530", "spanId": "0108b2d29b652107", "parentSpanId": "468e99f19f43d0db", "name": "QueryExecutor::localQuery()", "kind": 1, "startTimeUnixNano": "1689831889338531120", "endTimeUnixNano": "1689831889338737020", "attributes": [ { "key": "query.early_stop.pe_query_cnt", "value": { "stringValue": "1" } }, { "key": "query.early_stop.rt_query_cnt", "value": { "stringValue": "0" } }, { "key": "query.visit_pe_num", "value": { "stringValue": "1" } }, { "key": "query.early_stop.total_limit", "value": { "stringValue": "2" } }, { "key": "query.early_stop.hits", "value": { "stringValue": "2" } } ], "events": [], "links": [], "status": {} } ] } ], "schemaUrl": "https://opentelemetry.io/schemas/1.20.0" }' >> simple.log
```

* Output

```json
{
  "resource": {},
  "scopeSpans": [
    {
      "scope": {
        "name": "com.alibaba.cloud",
        "attributes": []
      },
      "spans": [
        {
          "traceId": "31646461386336653337343330356530",
          "spanId": "0108b2d29b652107",
          "parentSpanId": "468e99f19f43d0db",
          "name": "QueryExecutor::localQuery()",
          "kind": 1,
          "startTimeUnixNano": "1689831889338531120",
          "endTimeUnixNano": "1689831889338737020",
          "attributes": [
            {
              "key": "query.early_stop.pe_query_cnt",
              "value": {
                "stringValue": "1"
              }
            },
            {
              "key": "query.early_stop.rt_query_cnt",
              "value": {
                "stringValue": "0"
              }
            },
            {
              "key": "query.visit_pe_num",
              "value": {
                "stringValue": "1"
              }
            },
            {
              "key": "query.early_stop.total_limit",
              "value": {
                "stringValue": "2"
              }
            },
            {
              "key": "query.early_stop.hits",
              "value": {
                "stringValue": "2"
              }
            }
          ],
          "events": [],
          "links": [],
          "status": {}
        }
      ]
    }
  ],
  "schemaUrl": "https://opentelemetry.io/schemas/1.20.0"
}
```
