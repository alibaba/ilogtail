# Developing a Flusher Plugin

A Flusher plugin interacts with external systems to send data, and this guide will walk you through the process of developing a Flusher plugin, focusing on interface definition and a case study.

## Flusher Interface Definition

- **Init**: The initialization interface for the plugin, where a Flusher typically creates connections to external resources.

- **Description**: A one-sentence description of the plugin.

- **IsReady**: This interface is called by the plugin system before flushing data to check if the Flusher is ready to accept more. It takes project, logstore, and logstoreKey as parameters, and if `SetUrgent` has been called, the plugin should adjust accordingly to accept data promptly. See the [data structure](../data-structure.md) and [plugin system basics](../../principle/plugin-system.md) for more details.

- **Flush**: The main entry point for the plugin system to submit data to a Flusher instance. It sends data to external systems like SLS, console, or file. The function should usually return no error, as `IsReady` is called beforehand to ensure space for the next data.

- **SetUrgent**: Notifies the Flusher that it will soon be destroyed, allowing it to adapt to the system state, such as increasing output speed. This is called before other plugin types' `Stop` method, but currently, there's no meaningful implementation.

- **Stop**: Closes the Flusher and releases resources, including flushing any cached but unflushed data, releasing goroutines, file handles, or connections, and potentially more. After this, the Flusher should only have resources that can be garbage collected.

```go
type Flusher interface {
    // Init initializes system resources, like sockets or mutexes.
    Init(Context) error

    // Description returns a one-sentence description of the plugin.
    Description() string

    // IsReady checks if the Flusher is ready to accept more data.
    // @projectName, @logstoreName, @logstoreKey: metadata of the corresponding data.
    // Note: If SetUrgent is called, adjust so IsReady returns true promptly to accept more data and allow graceful shutdown.
    IsReady(projectName string, logstoreName string, logstoreKey int64) bool

    // Flush sends data to the destination, e.g., SLS, console, or file.
    // Expect to return no error most of the time, as IsReady will be called beforehand.
    Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error

    // SetUrgent indicates that the Flusher will soon be destroyed.
    // @flag: indicates if the main program (usually Logtail) will exit after this call.
    // Note: There may be more data to flush after SetUrgent, and if flag is true, these data will be passed through IsReady/Flush before the program exits.
    // Recommendation: set state flags in this method to guide the behavior of other methods.

    SetUrgent(flag bool)

    // Stop stops the Flusher and releases resources.
    // Perform cleanup tasks, such as:
    // 1. Flush cached but not flushed data. For aggregating or buffering Flushers, it's crucial to flush cached data now to avoid data loss.
    // 2. Release resources: goroutines, file handles, connections, etc.
    // 3. More may be needed, depending on the situation.
    // In summary, after this, the Flusher should only have resources that can be garbage collected.

    Stop() error
}
```

## Developing a Flusher Plugin

The development of a Flusher follows these steps:

1. **Create an issue**: Describe the plugin feature and discuss its feasibility with the community. If the review is approved, proceed to step 2.

2. **Implement the Flusher interface**: Use a sample pattern for guidance, and refer to the [flusher/stdout](https://github.com/alibaba/ilogtail/blob/main/plugins/flusher/stdout/flusher_stdout.go) for a detailed example.

3. **Register the plugin**: Implement the plugin in the [Flushers](https://github.com/alibaba/ilogtail/blob/main/plugin.go) using the `Init` method. The plugin's type in the JSON configuration (e.g., `plugin_type`) should start with "flusher_", as shown in the [flusher/stdout](https://github.com/alibaba/ilogtail/blob/main/plugins/flusher/stdout/flusher_stdout.go) example. Test the plugin with the [example.json](https://github.com/alibaba/ilogtail/blob/main/plugins/processor/addfields/example.json).

4. **Add the plugin to the configuration**: Include the plugin in the `common` section of the [plugin reference configuration file](https://github.com/alibaba/ilogtail/blob/main/plugins.yml). If the plugin is specific to a system, add it to the `linux` or `windows` section.

5. **Write unit tests or E2E tests**: Follow the instructions in [writing unit tests](../test/unit-test.md) and [writing E2E tests](../test/e2e-test.md).

6. **Lint the code**: Use `make lint` to check coding conventions.

7. **Submit a pull request**.
