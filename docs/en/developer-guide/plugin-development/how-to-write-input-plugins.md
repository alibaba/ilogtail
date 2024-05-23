# How to Develop Input Plugins

Before diving into the development of Input plugins, it's essential to understand the Input plugin interface. There are two types of Input plugins: MetricInput and ServiceInput. MetricInput is suitable for global control over data collection frequency, typically for metric gathering. ServiceInput is more appropriate for receiving external inputs, such as SkyWalking trace data pushes.

## MetricInput Interface Definition

- `Init()`: The initialization interface for the plugin, where it can perform checks and acquire a context object, `Context`.

- Return values: The first parameter is the call interval (in milliseconds), and the plugin system will use this value as the data collection frequency. If the return value is 0, the plugin system will use the global data collection period.

- The second parameter is an error flag, which indicates any errors that occurred during initialization. If an error occurs, the plugin instance will be ignored, and the plugin system won't collect data from it.

- `Description()`: Returns a one-sentence description of the Input plugin.

- `Collect(Collector)`: The most critical interface for MetricInput, where the plugin system retrieves data from the plugin instance. This method is periodically called, and the plugin adds metrics to the provided `Collector`.

```go
// MetricInput ...

type MetricInput interface {
    // Init called for initializing system resources, like sockets or mutexes.
    // return call interval (ms) and error flag. If interval is 0, use default interval.
    Init(Context) (int, error)

    // Description returns a one-sentence description of the Input.
    Description() string

    // Collect takes in an accumulator and adds the metrics that the Input gathers.
    // This is called every "interval".
    Collect(Collector) error
}
```

## Developing a MetricInput Plugin

The development process for a MetricInput plugin involves the following steps:

1. Create an issue to describe the plugin's functionality, and the community will review its feasibility. If the review is approved, proceed to step 2.

2. Implement the MetricInput interface, using a sample pattern as a guide. Detailed examples can be found in [input/example/metric](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/metric_example.go), and you can test the plugin with [example.json](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/metric_example_input.json).

3. Register the plugin in the [MetricInputs](https://github.com/alibaba/ilogtail/blob/main/plugin.go) using the `init` method. The plugin type in the JSON configuration must start with "metric_", as shown in the [example](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/metric_example.go).

4. Add the plugin to the [global plugin definition center](https://github.com/alibaba/ilogtail/blob/main/plugins/all/all.go), or to the [Linux plugin definition center](https://github.com/alibaba/ilogtail/blob/main/plugins/all/all_linux.go) or [Windows plugin definition center](https://github.com/alibaba/ilogtail/blob/main/plugins/all/all_windows.go) if it's only for a specific system.

5. Perform unit tests or E2E tests, following the instructions in [How to Use Unit Tests](../test/unit-test.md) and [How to Use E2E Tests](../test/e2e-test.md).

6. Run `make lint` to check code conventions.

7. Submit a Pull Request.

## ServiceInput Interface Definition

- `Init/Description()`: Similar to the MetricInput interface, but the interval parameter in `Init` is irrelevant for ServiceInput, as it won't be called periodically.

- `Start()`: Since ServiceInput is designed for a resident-type input, the plugin system creates a separate goroutine for each instance, where this interface is called to start data collection. The `Collector` still serves as a data pipeline between the plugin instance and the plugin system.

- `Stop()`: Provides a way to stop the service. The plugin must implement this interface correctly to avoid hanging the plugin system and causing Logtail to exit improperly.

```go
// ServiceInput ...

type ServiceInput interface {
    // Init called for initializing system resources, like sockets or mutexes.
    // return call interval (ms) and error flag. If interval is 0, use default interval.
    Init(Context) (int, error)

    // Description returns a one-sentence description of the Input.
    Description() string

    // Start starts the ServiceInput's service.
    Start(Collector) error

    // Stop stops the service and closes any necessary channels or connections.
    Stop() error
}
```

## Developing a ServiceInput Plugin

The development process for a ServiceInput plugin involves:

1. Create an issue to describe the plugin's functionality, and the community will review its feasibility. If approved, proceed to step 2.

2. Implement the ServiceInput interface, using a sample pattern as a guide. Detailed examples can be found in [input/example/service](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/service_example.go), and you can test the plugin with [example.json](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/service_example_input.json).

3. Register the plugin in the [ServiceInputs](https://github.com/alibaba/ilogtail/blob/main/plugin.go) using the `init` method. The plugin type in the JSON configuration must start with "service_", as shown in the [example](https://github.com/alibaba/ilogtail/blob/main/plugins/input/example/service_example.go).

4. Add the plugin to the `common` section in the [plugin reference configuration file](https://github.com/alibaba/ilogtail/blob/main/plugins.yml), or to the `linux` or `windows` section if it's specific to a system.

5. Perform unit tests or E2E tests, following the instructions in [How to Use Unit Tests](../test/unit-test.md) and [How to Use E2E Tests](../test/e2e-test.md).

6. Run `make lint` to check code conventions.

7. Submit a Pull Request.
