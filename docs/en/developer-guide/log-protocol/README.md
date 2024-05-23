# Log Protocol

iLogtail's log data by default communicates with external systems using a custom SLs protocol. However, it supports converting log data between SLs and other standard or custom protocols. Additionally, iLogtail offers different encoding options for specific protocols.

The supported protocols and their corresponding encoding methods for iLogtail's log data are listed in the table below, categorized into custom and standard protocols:

| Protocol Type | Protocol Name                                                                                           | Supported Encodings |
|--------------|---------------------------------------------------------------------------------------------------------|-------------------|
| Standard     | [SLs Protocol](./protocol-spec/sls.md)                                                                   | json, protobuf     |
| Custom       | [Single Message Protocol](./protocol-spec/custom_single.md)                                             | json              |
| Standard     | [Influxdb Protocol](https://docs.influxdata.com/influxdb/v1.8/write_protocols/line_protocol_reference/) | custom            |
| Byte Stream  | [Raw Protocol](./protocol-spec/raw.md)                                                                   | custom            |
