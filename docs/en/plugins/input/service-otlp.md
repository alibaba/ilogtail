# Open Telemetry Service Input

## Introduction

The `service_otlp` `input` plugin implements the `ServiceInputV1` and `ServiceInputV2` interfaces, accepting `OpenTelemetry log/metric/trace protocol` over HTTP or gRPC requests, and converting them to either SLSProto or PipelineGroupEvents. Currently, it does not support converting OTLP traces to SLSProto format.

## Version

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter               | Type      | Required | Description                                                                                     |
|-------------------------|----------|----------|--------------------------------------------------------------------------------------------------|
| Type                    | String   | Yes      | Plugin type, always set to `service_otlp`.                                                               |
| Protocols                | Struct   | Yes      | <p>Protocol to receive</p>                                                                       |
| Protocols.GRPC           | Struct   | No       | Whether to enable the gRPC server.                                                                   |
| Protocols.GRPC.Endpoint   | string   | No       | <p>gRPC server address. Default is `0.0.0.0:4317`.</p>                                                  |
| Protocols.GRPC.MaxRecvMsgSizeMiB | int   | No       | Maximum message size for gRPC server.                                                               |
| Protocols.GRPC.MaxConcurrentStreams | int   | No       | Maximum concurrent streams for gRPC server.                                                         |
| Protocols.GRPC.ReadBufferSize | int   | No       | gRPC server read buffer size.                                                                      |
| Protocols.GRPC.WriteBufferSize | int   | No       | gRPC server write buffer size.                                                                     |
| Protocols.GRPC.Compression | string   | No       | gRPC server compression algorithm, e.g., gzip.                                                       |
| Protocols.GRPC.Decompression | string   | No       | gRPC server decompression algorithm, e.g., gzip.                                                     |
| Protocols.GRPC.TLSConfig | Struct   | No       | gRPC server TLS configuration.                                                                      |
| Protocols.HTTP           | Struct   | No       | Whether to enable the HTTP server.                                                                   |
| Protocols.HTTP.Endpoint   | string   | No       | <p>HTTP server address. Default is `0.0.0.0:4318`.</p>                                                  |
| Protocols.HTTP.MaxRecvMsgSizeMiB | int   | No       | Maximum message size for HTTP server. Default is `64 MiB`.                                          |
| Protocols.HTTP.ReadTimeoutSec | int   | No       | <p>HTTP request read timeout. Default is `10s`.</p>                                                 |
| Protocols.HTTP.ShutdownTimeoutSec | int   | No       | <p>HTTP server shutdown timeout. Default is `5s`.</p>                                              |

## Examples

* Only receive gRPC requests.

```yaml
enable: true
version: v2
inputs:
  - Type: service_otlp
    Protocols:
      GRPC:
```

* Accept HTTP/gRPC requests using the default OTLP ports. gRPC: 4317, HTTP: 4318.

```yaml
enable: true
version: v2
inputs:
  - Type: service_otlp
    Protocols:
      GRPC:
      HTTP:
```

* Full configuration example.

```yaml
enable: true
version: v2
inputs:
  - Type: service_otlp
    Protocols:
      GRPC:
        Endpoint: 0.0.0.0:4317
        MaxRecvMsgSizeMiB: 64
        MaxConcurrentStreams: 100
        ReadBufferSize: 1024
        WriteBufferSize: 1024
      HTTP:
        Endpoint: 0.0.0.0:4318
        MaxRecvMsgSizeMiB: 64
        ReadTimeoutSec: 10
        ShutdownTimeoutSec: 5
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```
