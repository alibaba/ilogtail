# Protocol conversion

When developing Flusher plug-ins, users often need to convert log data from the sls protocol to other protocols. After extending the Metric data model, the Flusher plug-in of version v2 also needs to support the scenario of converting data from PipelineGroupEvents to other protocols.To speed up the development of plug-ins, iLogtail provides a common protocol conversion module. You only need to specify the name and encoding method of the target protocol to obtain the encoded byte stream.

## Converter structure

The specific implementation of the log protocol converter is 'Converter' structure, which is defined as follows:

```Go
type Converter struct {
    Protocol             string
    Encoding             string
    TagKeyRenameMap      map[string]string
    ProtocolKeyRenameMap map[string]string
}
```

The meanings of each field are as follows:

-'Protocol': Protocol name
-'Encoding': encoding method
-'TagKeyRenameMap': the tag field Key renaming table can be used to rename the Key of the LogTag field in the sls protocol. The default value of the LogTag Key retained by the system is shown in the appendix. To delete a tag, you only need to rename the tag to an empty string.
-'ProtocolKeyRenameMap': The Protocol field Key renaming table, which can be used to rename the protocol field Key. It is only available in partial encoding mode. For more information, see Appendix.

You can use the following functions to create an 'Converter' object instance:

```Go
func NewConverter(protocol, encoding string, tagKeyRenameMap, protocolKeyRenameMap map[string]string) (*Converter, error)
```

The optional values of the 'protocol' field and 'encoding' are shown in the appendix.

## Conversion method

The 'Converter' structure supports the following methods:

```Go
func (c *Converter) Do(logGroup *sls.LogGroup) (logs interface{}, err error)

func (c *Converter) ToByteStream(logGroup *sls.LogGroup) (stream interface{}, err error)

func (c *Converter) DoWithSelectedFields(logGroup *sls.LogGroup, targetFields []string) (logs interface{}, values []map[string]string, err error)

func (c *Converter) ToByteStreamWithSelectedFields(logGroup *sls.LogGroup, targetFields []string) (stream interface{}, values []map[string]string, err error)

func (c *Converter) ToByteStreamWithSelectedFieldsV2(groupEvents *models.PipelineGroupEvents, targetFields []string) (stream interface{}, values []map[string]string, err error)
```

The usage of each method is as follows:

- 'Do': For a given log group 'logGroup', each log in the sls log group is converted to the data structure corresponding to the target protocol according to the protocol set in 'Converter', and the results are assembled in 'logs;
- 'ToByteStream': based on the 'Do' method, encode the converted logs to form byte streams according to the encoding format set in 'Converter', and assemble the results in 'stream;
- 'DoWithSelectedFields': based on the log conversion that is consistent with the 'Do' method, find the value of the specified field in the 'targetFields' array in each log and log group tag, and save the result in the 'values' array as a map.If this field is not found, the field is not included in each map of 'values. The format of each string in 'targetFields' is as follows:
  - If you need to obtain the value of a log field, the string is named "content.XXX", where XXX is the key name of the field;
  - To obtain the value of a log tag, the string is named as tag.XXX, where XXX is the key name of the field.
- 'ToByteStreamWithSelectedFields': similar to the 'DoWithSelectedFields' method, based on the log conversion that is consistent with the 'ToByteStream' method, you can find the value of the specified field in the 'targetFields' array in each log and log group tag,The result is saved in the 'values' array in the form of map. The format of each string in 'targetFields' is the same as that in the preceding figure.
- 'ToByteStreamWithSelectedFieldsV2': The function is the same as the 'ToByteStreamWithSelectedFields' method, which only acts on the protocol conversion of the v2 data template.

## Procedure

The following example shows how to use 'Converter' to convert logs:

1. Introduce the 'protocol' package to the current file:

    ```Go
    import "github.com/alibaba/ilogtail/pkg/protocol"
    ```

2. Use the selected conversion parameters to create an 'Converter' object instance:

    ```Go
    c, err := protocol.NewConverter(...)
    ```

3. Given a log Group,

    - If you only need to convert the log format, call the 'Do' method of 'Converter' to obtain the converted data structure:

        ```Go
        convertedLogs, err := c.Do(logGroup)
        ```

    - To convert and encode the log format, call the 'ToByteStream' method of 'Converter' to obtain the encoded byte stream:

        ```Go
        serializedLogs, err := c.ToByteStream(logGroup)
        ```

    - If you want to obtain the values of some fields in the log while converting the log format, call the 'DoWithSelectedFields' method of 'Converter' to obtain the converted data structure and the values of the selected log fields at the same time:

        ```Go
        convertedLogs, targetValues, err := c.DoWithSelectedFields(logGroup, selectedFields)
        ```

    - If you want to obtain the values of some fields in the log while converting and encoding the log format, call the 'ToByteStreamWithSelectedFields' method of 'Converter' to obtain the values of the encoded byte stream and the selected log fields at the same time:

        ```Go
        serializedLogs, targetValues, err := c.ToByteStreamWithSelectedFields(logGroup, selectedFields)
        ```

## Example

Assume that logs need to be converted from the sls protocol to a single protocol. If the encoding method is json, the host.name key in the tag is changed to hostname, and the time key in the protocol is changed to @ timestamp, the code for creating 'Converter' is as follows:

```Go
c, err := protocol.NewConverter("custom_single", "json", map[string]string{"host.name":"hostname"}, map[string]string{"time", "@timestamp"})
```

## Appendix

- Optional protocol name

| Protocol name | Meaning |
|------------------------------------------| ------ |
| custom_single | Single protocol |
| custom_single_flatten | Single protocol, data tiled, such as the json message body written to kafka |
| influxdb              | Influxdb protocol                               |
| raw | The original Byte stream protocol, which only supports protocol conversion of ByteArray-type events in V2. |

- Optional encoding method

| Encoding method | Meaning |
| ------ | ------ |
| json | json encoding method |
| protobuf | protobuf encoding method |
| custom | custom encoding method |

- Default value of the LogTag Key retained by the system in the sls protocol:

| Field name | Description |
| ------ | ------ |
| host.ip | The ip address of the machine or container to which the iLogtail belongs. |
| log.topic | The topic of the log. |
| log.file.path | The path of the collected file. |
| host.name | The host name of the machine or container to which the iLogtail belongs. |
| k8s.node.ip | iLogtail ip address of the K8s node where the container is located |
| k8s.node.name | iLogtail name of the K8s node where the container is located |
| k8s.namespace.name | The K8s namespace to which the business container belongs |
| k8s.pod.name | The name of the K8s Pod to which the business container belongs |
| k8s.pod.ip | The K8s Pod ip to which the business container belongs |
| k8s.pod.uid | The K8s Pod uid to which the business container belongs. |
| (k8s.)container.name | The name of the business container. If you are in a K8s environment, add the prefix "k8s" |
| (k8s.)container.ip | The ip address of the service container. If you are in a K8s environment, add the prefix "k8s" |
| (k8s.)container.image.name | The name of the image used to create a business container. If you are in a K8s environment, add the prefix "k8s" |

- The encoding format that can be used to rename the protocol field Key: json
