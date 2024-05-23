# sls protocol

The Protobuf mode of the sls protocol is defined as follows:

## Content & LogTag

Transfer data fields and tags, simple key/value pairs.

```protobuf
message Content
{
  required string Key = 1;
  required string Value = 2;
}

message LogTag
{
  required string Key = 1;
  required string Value = 2;
}
```

## Log

Log is the data type of a single Log. The Time field is the Log Time. The Contents field maintains the Log content and consists of a key/value list.

```protobuf
message Log
{
  required uint32 Time = 1;// UNIX Time Format
  repeated Content Contents = 2;
}
```

## LogGroup

LogGroup (log group) is the packaging of multiple logs:

- Logs: Contains all Logs.
- Category: Log service Logstore, similar to Kafka Independent cluster, independent unit of data storage.
- Topic: a log service Topic. A Category can be divided into multiple topics. If not specified, the topic is an empty string, which can be similar to the Topic concept in a Kafka Independent cluster.
- Source/MachineUUID: the information of the node where the iLogtail is located. The former is the IP address and the latter is the UUID.
- LogTags: the common tag of all logs, which also consists of key/value lists.

```protobuf
message LogGroup
{
  repeated Log Logs = 1;
  optional string Category = 2;
  optional string Topic = 3;
  optional string Source = 4;
  optional string MachineUUID = 5;
  repeated LogTag LogTags = 6;
}
```
