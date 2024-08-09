# ServiceInput Example Plugin

# # Introduction

The `service_input_example` serves as a reference example for writing a `ServiceInput` class plugin, which can receive simulated HTTP requests on a specified port. [Source Code](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/service_example.go)

# # Version

[Stable](../stability-level.md)

# # Configuration Parameters

| Parameter | Type, Default Value | Description |

| --- | --- | --- |

| Type | String, No Default (Required) | Plugin type, always set to `service_input_example`. |

| Address | String, `:19000` | Listening port for incoming requests. |

# # Example

* Configuration

```yaml

enable: true

inputs:

  - Type: service_input_example

flushers:

  - Type: flusher_stdout

    OnlyStdout: true  

```

* Input

```bash

curl --header "test:val123" http://127.0.0.1:19000/data

```

* Output

```json

{
    "test": "val123",
    "__time__": "1658495321"
}
```
