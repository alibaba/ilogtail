# raw Protocol

The raw protocol's data structure in memory is defined as follows:

## ByteArray & GroupInfo

A raw byte stream for transmitting data, which is a concrete implementation of `PipelineEvent`.

```go
type ByteArray []byte
```

Metadata and tag information for the data, consisting of simple key-value pairs.

```go
type GroupInfo struct {
    Metadata Metadata
    Tags     Tags
}
```

## PipelineGroupEvents

The complete data package.

```go
type PipelineGroupEvents struct {
    Group  *GroupInfo
    Events []PipelineEvent
}
```
