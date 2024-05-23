# Kafka

## Introduction

The `service_kafka` `input` plugin implements the `ServiceInputV1` and `ServiceInputV2` interfaces, designed to collect messages from Kafka topics for monitoring purposes.

## Versioning

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter                  | Type      | Required | Description                                                                                                                                   |
|----------------------------|---------|----------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| Type                       | String   | Yes      | Plugin type, set to `service_kafka`.                                                                                                          |
| Format                     | String   | No       | Added in ilogtail 1.6.0, only applicable to ServiceInputV2. Supports formats: `raw`, `prometheus`, `otlp_metricv1`, `otlp_tracev1`. Default: `raw` |
| Version                    | String   | Yes      | Kafka cluster version number.                                                                                                                   |
| Brokers                    | Array    | Yes      | List of Kafka server addresses.                                                                                                                  |
| ConsumerGroup              | String   | Yes      | Name of the Kafka consumer group.                                                                                                               |
| Topics                     | Array    | Yes      | List of topics to subscribe for consumption.                                                                                                      |
| ClientID                   | String   | Yes      | User ID for consuming Kafka messages.                                                                                                          |
| Offset                     | String   | No       | Initial offset for consuming messages, options are `oldest` or `newest`. Defaults to `oldest` if not provided.                                   |
| MaxMessageLen (Deprecated) | Integer  | No       | Maximum allowed message length in bytes, range 1-524288. Defaults to 524288 (512KB) if not provided. ilogtail 1.6.0 no longer uses this parameter. |
| SASLUsername              | String   | No       | SASL username.                                                                                                                                  |
| SASLPassword              | String   | No       | SASL password.                                                                                                                                  |
| Assignor                   | String   | No       | Partition assignment strategy for the consumer group, options: `range`, `roundrobin`, `sticky`. Default: `range`                                |
| DisableUncompress         | Boolean  | No       | Added in ilogtail 1.6.0, disables message decompression. Default: `false`. Only applicable to `raw` format.                                   |
| FieldsExtend               | Boolean  | No       | Whether to support non-integer data types (e.g., String). Currently only relevant for influxdb format with additional types like String, Bool. Only v2. |
| ...                        | ...      | ...      | ...                                                                                                                                             |

## Example

Collect messages from Kafka servers at 172.xx.xx.48 and 172.xx.xx.34, consuming topics topicA and topicB, with a Kafka cluster version of 2.1.1 and a consumer group named `test-group`. Use default values for all other parameters.

* Input (v1)

```json
{
  "payload": "foo"
}
```

* Config（v1）

```yaml
enable: true
inputs:
  - Type: service_kafka
    Version: 2.1.1
    Brokers: 
        - 172.xx.xx.48
        - 172.xx.xx.34
    ConsumerGroup: test-group
    Topics:
        - topicA
        - topicB
    ClientID: sls
flushers:
  - Type: flusher_stdout
    OnlyStdout: true  
```

* Output (v1)

```json
{"payload": "foo"}
```

## Collector Configuration (v2)

`v2` is a new implementation introduced in ilogtail 1.6.0, supporting configuration of data formats like `raw`, `prometheus`, `otlp_metricv1`, and `otlp_tracev1`. Other configuration parameters have been updated, as detailed in the table above. If no specific format is needed, the default `v1` configuration should suffice.

```yaml
enable: true
version: v2
inputs:
  - Type: service_kafka
    Version: 2.1.1
    Brokers:
      - 172.xx.xx.48
      - 172.xx.xx.34
    ConsumerGroup: test-group
    Topics:
      - topicA
      - topicB
    ClientID: sls
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output (v2)

```json
{"eventType": "byteArray", "name": "", "timestamp": 0, "observedTimestamp": 0, "tags": {}, "byteArray": "{\"payload \": \"foo \"}"}
```
