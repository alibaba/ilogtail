# Http Server

## Introduction

The 'service_http_server' 'input' plug-in can receive requests from unix socket, http/https, and tcp, and supports multiple protocols such as sls and otlp.

## Version

[Stable](../stability-level.md)

## Configure parameters

| Parameter | Type | Required | Description |
|--------------------|-------------------|------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Type | String | Yes | Plug-in Type, fixed to 'service_http_server' |
| Format | String | No | <p> Data Format. </p> <p> Supported formats: 'sls', 'prometheus', 'influxdb', 'otlp_logv1 ', 'otlp_metricv1', 'pyroscopical, 'statsd'</p> <p> Supported formats for v2: 'raw', 'prometheus', 'otlp_logv1 ', 'otlp_trac' data <1> |
| Address | String | No | <p> The Listening Address. </p> <p> </p> |
| Path | String | No | <p> The receiving endpoint. If the Format is 'otlp_logv1 ', the default endpoint is'/v1/logs '.</p><p></p>                                                                                                                                                        |
| ReadTimeoutSec | String | No | <p> Read timeout. </p><p> The default value is '10s '. </p> |
| ShutdownTimeoutSec | String | No | <p> Disable timeout. </p><p> The default value is '5S '. </p> |
| MaxBodySize | String | No | <p> The maximum size of the transmission body. </p><p> The default value is '64k '. </p> |
| UnlinkUnixSock | String | No | <p> Whether to forcibly release the listener if the listener address is unix socket before startup. </p><p> The default value is 'true '.</p>                                                                                                                                                             |
| FieldsExtend | Boolean | No | <p> Whether data types other than integer (such as String) are supported </p><p> currently only valid for influxdb Format with additional types such as String and Bool </p> |
| QueryParams | []String | No | The request parameters to be parsed to the Group.Metadata. <p> The parsing result is put into the KeyValue in Metadata.The default value is '[]', that is, it is not parsed. </p><p> valid only for version v2 </p> |
| QueryParamPrefix | String | No | The key prefix to be added when parsing request parameters, such as '_query_param_'. <p> prefixes are directly concatenated before each QueryParam,No additional connectors. The default value is null, that is, no prefix is added. </p><p> valid only for version v2 </p> |
| HeaderParams | []String | No | The header parameter that needs to be parsed to the Group.Metadata. <p> The parsing result is put into the KeyValue in Metadata.The default value is '[]', that is, it is not parsed. </p><p> valid only for version v2 </p> |
| HeaderParamPrefix | String | No | The key prefix to be added when parsing Header parameters, such as '_header_param_'. <p> prefixes are directly concatenated before each HeaderParam,No additional connectors. The default value is null, that is, no prefix is added. </p><p> valid only for version v2 </p> |
| DisableUncompress | Boolean | No | Disable decompression of request data. The default value is: 'false'<p> valid only for Raw Format </p><p> valid only for v2 </p> |
| Tags | map[String]String | No | Output data carries Tags by default <p> valid only for v1 </p> |
| DumpData | Boolean | No | [development] stores the received request in a local file. The default value is 'false' |
| DumpDataKeepFiles | Int | No | [for development] Dump the number of files to be retained. The files are rolled by hour. The default value of this parameter is 5, which indicates that the Dump parameter is retained for 5 hours. |
| AllowUnsafeMode | Boolean | No | Whether to allow Decode in unsafe mode. If this mode is enabled, Decoder may use go unsafe technology to accelerate decoding,Valid only when Format = prometheus (note: Exemplar and Histogram are not supported) |

## Sample

### Receive OTLP logs

* Collection configuration

```yaml
enable: true
inputs:
  - Type: service_http_server
Format: "otlp_logv1"
    Address: "http://127.0.0.1:12345"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true  
```

* Input

Put to use

Use opentelemetry-java-sdk，base on [ExampleConfiguration.java](https://github.com/open-telemetry/opentelemetry-java-docs/blob/main/otlp/src/main/java/io/opentelemetry/example/otlp/ExampleConfiguration.java)、[OtlpExporterExample.java](https://github.com/open-telemetry/opentelemetry-java-docs/blob/main/otlp/src/main/java/io/opentelemetry/example/otlp/OtlpExporterExample.java)

Perform the following code modification.

```java
  static OpenTelemetrySdk initHTTPOpenTelemetry() {
    // Include required service.name resource attribute on all spans and metrics
    Resource resource =
        Resource.getDefault()
            .merge(Resource.builder().put(SERVICE_NAME, "OtlpExporterExample").build());

    OpenTelemetrySdk openTelemetrySdk =
        OpenTelemetrySdk.builder()
            .setLogEmitterProvider(
                SdkLogEmitterProvider.builder()
                    .setResource(resource)
                    .addLogProcessor(SimpleLogProcessor
                        .create(OtlpHttpLogExporter
                                .builder()
                                .setEndpoint("http://127.0.0.1:12345/v1/logs")
                                .build()))
                    .build())
            .buildAndRegisterGlobal();

    Runtime.getRuntime()
        .addShutdownHook(new Thread(openTelemetrySdk.getSdkLogEmitterProvider()::shutdown));

    return openTelemetrySdk;
  }
```

'OtlpExporterExample' constructs data.

```java
  OpenTelemetrySdk openTelemetry = ExampleConfiguration.initHTTPOpenTelemetry();

  LogEmitter logger = openTelemetry.getSdkLogEmitterProvider().get("io.opentelemetry.example");
  logger
    .logRecordBuilder()
    .setBody("log body1")
    .setAllAttributes(
      Attributes.builder()
                  .put("k1", "v1")
                  .put("k2", "v2").build())
    .setSeverity(Severity.INFO)
    .setSeverityText("INFO")
    .setEpoch(Instant.now())
    .setContext(Context.current())
    .emit();
```

* Output

```json
{
    "time_unix_nano": "1663913736115000000",
    "severity_number": "9",
    "severity_text": "INFO",
    "content": "log body1",
    "attributes": "{\"k1\":\"v1\",\"k2\":\"v2\"}",
    "resources": "{\"service.name\":\"OtlpExporterExample\",\"telemetry.sdk.language\":\"java\",\"telemetry.sdk.name\":\"opentelemetry\",\"telemetry.sdk.version\":\"1.18.0\"}",
    "__time__": "1663913736"
}
```

### Receive OTLP Logs/Metrics/Traces (v2)

Note: currently, v2 pipeline does not support otlp logs requests, and data is discarded.

* Collection configuration

```yaml

enable: true
version: v2
inputs:
  - Type: service_http_server
Format: "otlp_logv1"
    Address: "http://127.0.0.1:12344"
  - Type: service_http_server
Format: "otlp_metricv1"
    Address: "http://127.0.0.1:12345"
  - Type: service_http_server
Format: "otlp_tracev1"
    Address: "http://127.0.0.1:12346"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true  
```

### Receive byte stream data

* Collection configuration

```yaml
enable: true
version: v2
inputs:
  - Type: service_http_server
    Format: "raw"
    Address: "http://127.0.0.1:12345"
    QueryParams:
      - "QueryKey"
QueryParamPrefix: "_ param_prefix _"
    HeaderParams:
      - "HeaderKey"
HeaderParamPrefix: "_ header_prefix _"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* Input

Manually construct a request

```shell
curl --location --request POST 'http://127.0.0.1:12345?QueryKey=queryValue' --header 'HeaderKey: headerValue' --header 'Content-Type: text/plain' --data 'test_measurement,host=server01,region=cn value=0.5'
```

* Output

```plain
[Event] event 1, metadata map[_header_prefix_HeaderKey:headerValue _param_prefix_QueryKey:queryValue], tags map[__hostname__:579ce1e01dea]

{
    "eventType":"byteArray",
    "name":"",
    "timestamp":0,
"ObservedTimestamp":0,
    "tags":{
    },
    "byteArray":"test_measurement,host=server01,region=cn value=0.5"
}
```

### Receive Pyroscope Agent data

* [Agent](https://pyroscope.io/docs/agent-overview/) compatibility description

| Agent | Protocol | Compatibility |
|:-------------------:|:----------:|:----:|
| Pyroscopde/nodjs | pprof | |
|   pyroscopde/.net   |   pprof    |  Yes   |
| pyroscopde/.net-new |   pprof    |  Yes   |
|   pyroscopde/java   |    JFR     |  Yes   |
|    pyroscopde/go    |   pprof    |  Yes   |
|   pyroscopde/php    |  raw tire  |  Yes   |
|   pyroscopde/ebpf   |  raw tire  |  Yes   |
|   pyroscopde/rust   | raw groups |  Yes   |
|   pyroscopde/ruby   | raw groups |  Yes   |
|  pyroscopde/python  | raw groups |  Yes   |

* Collection configuration
* Use v1 to express data transfer using protocol.Log *

```yaml
enable: true
version: v1
inputs:
  - Type: service_http_server
    Format: "pyroscope"
    Address: "http://:4040"
    Endpoint: "/ingest"
    Cluster: "sls-mall"
    TagsInGroup: false
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* Input

1. Enter [Pyroscope Golang Example](https://github.com/pyroscope-io/pyroscope/tree/main/examples/golang-push)
2. Build Pyroscope Golang Example

    ```shell
    go build -o main .
    ```

3. Run Pyroscope Golang Example

    ```shell
    ./main
    ```

* Output

```text
2023-02-21 14:15:20 {"name":"runtime.malg /Users/evan/sdk/go1.19.4/src/runtime/proc.go","stack":"runtime.newproc1 /Users/evan/sdk/go1.19.4/src/runtime/proc.go\nruntime.newproc.func1 /Users/evan/sdk/go1.19.4/src/runtime/proc.go\nruntime.systemstack /Users/evan/sdk/go1.19.4/src/runtime/asm_amd64.s","stackID":"afb4871c4cb30267","language":"go","type":"profile_mem","dataType":"CallStack","durationNs":"0","profileID":"e3620581-930d-4c51-8e52-cb752e75f1be","labels":"{\"__name__\":\"simple.golang.app\",\"_sample_rate_\":\"100\",\"cluster\":\"s=s-mall\"}","units":"count","valueTypes":"alloc_objects","aggTypes":"sum","val":"1260.00","__time__":"1676960120"}
2023-02-21 14:15:20 {"name":"runtime.malg /Users/evan/sdk/go1.19.4/src/runtime/proc.go","stack":"runtime.newproc1 /Users/evan/sdk/go1.19.4/src/runtime/proc.go\nruntime.newproc.func1 /Users/evan/sdk/go1.19.4/src/runtime/proc.go\nruntime.systemstack /Users/evan/sdk/go1.19.4/src/runtime/asm_amd64.s","stackID":"afb4871c4cb30267","language":"go","type":"profile_mem","dataType":"CallStack","durationNs":"0","profileID":"e3620581-930d-4c51-8e52-cb752e75f1be","labels":"{\"__name__\":\"simple.golang.app\",\"_sample_rate_\":\"100\",\"cluster\":\"s=s-mall\"}","units":"bytes","valueTypes":"alloc_space","aggTypes":"sum","val":"524496.00","__time__":"1676960120"}
2023-02-21 14:15:20 {"name":"runtime.malg /Users/evan/sdk/go1.19.4/src/runtime/proc.go","stack":"runtime.newproc1 /Users/evan/sdk/go1.19.4/src/runtime/proc.go\nruntime.newproc.func1 /Users/evan/sdk/go1.19.4/src/runtime/proc.go\nruntime.systemstack /Users/evan/sdk/go1.19.4/src/runtime/asm_amd64.s","stackID":"afb4871c4cb30267","language":"go","type":"profile_mem","dataType":"CallStack","durationNs":"0","profileID":"e3620581-930d-4c51-8e52-cb752e75f1be","labels":"{\"__name__\":\"simple.golang.app\",\"_sample_rate_\":\"100\",\"cluster\":\"s=s-mall\"}","units":"count","valueTypes":"inuse_objects","aggTypes":"sum","val":"1260.00","__time__":"1676960120"}
2023-02-21 14:15:20 {"name":"compress/flate.(*compressor).init /Users/evan/sdk/go1.19.4/src/compress/flate/deflate.go","stack":"compress/flate.NewWriter /Users/evan/sdk/go1.19.4/src/compress/flate/deflate.go\ncompress/gzip.(*Writer).Write /Users/evan/sdk/go1.19.4/src/compress/gzip/gzip.go\nruntime/pprof.(*profileBuilder).flush /Users/evan/sdk/go1.19.4/src/runtime/pprof/proto.go\nruntime/pprof.(*profileBuilder).pbSample /Users/evan/sdk/go1.19.4/src/runtime/pprof/proto.go\nruntime/pprof.(*profileBuilder).build /Users/evan/sdk/go1.19.4/src/runtime/pprof/proto.go\nruntime/pprof.profileWriter /Users/evan/sdk/go1.19.4/src/runtime/pprof/pprof.go","stackID":"d4e9448662480cdb","language":"go","type":"profile_mem","dataType":"CallStack","durationNs":"0","profileID":"e3620581-930d-4c51-8e52-cb752e75f1be","labels":"{\"__name__\":\"simple.golang.app\",\"_sample_rate_\":\"100\",\"cluster\":\"s=s-mall\"}","units":"count","valueTypes":"alloc_objects","aggTypes":"sum","val":"177.00","__time__":"1676960120"}
```
