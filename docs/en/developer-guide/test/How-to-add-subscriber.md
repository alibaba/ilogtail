# How to Write a Subscriber Plugin

A Subscriber plugin is a component in the testing engine that receives data and forwards it to a Validator for further processing. If you're developing a new output plugin for iLogtail, you must create a corresponding Subscriber to fetch data from the storage unit associated with your output plugin and use it in integration tests.

## Subscriber Interface Definition

- `Name()`: Returns the name of the subscriber.
- `Start()`: Starts the subscriber, continuously fetching data from the target storage unit, transforming it, and sending it to the channel returned by `SubscribeChan()`.
- `Stop()`: Stops the subscriber.
- `SubscribeChan()`: Returns a channel to send received data to the validator. The channel's data type should correspond to the specific details in the [LogGroup](../../docs/cn/developer-guide/data-structure.md) documentation.
- `FlusherConfig()`: Returns the default flusher configuration for the iLogtail container associated with this subscriber. It can return an empty string directly.

```go
type Subscriber interface {
    doc.Doc

    // Name returns the name of the subscriber
    Name() string

    // Start starts the subscriber
    Start() error

    // Stop stops the subscriber
    Stop()

    // SubscribeChan returns the channel used to transmit received data to the validator
    SubscribeChan() <-chan *protocol.LogGroup

    // FlusherConfig returns the default flusher config for the iLogtail container corresponding to this subscriber
    FlusherConfig() string
}
```

## Subscriber Development Workflow

1. Create a new `<subscriber_name>.go` file in the `./test/engine/subscriber` directory and implement the Subscriber interface. You can refer to the default subscriber [grpc](../engine/subscriber/grpc.go) for a sample.

2. In the `init()` function of this file, register your plugin using the `RegisterCreator(name string, creator Creator)` function. The `Creator` is a function with the prototype `func(spec map[string]interface{}) (Subscriber, error)`, like in the default subscriber [grpc](../engine/subscriber/grpc.go).

3. Configure the relevant parameters in the `subscriber` block of the engine configuration file, ilogtail-e2e.yaml.
