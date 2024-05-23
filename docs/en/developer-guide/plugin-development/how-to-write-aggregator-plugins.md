# Developing an Aggregator Plugin

The Aggregator plugin bundles input data by applying certain rules. This guide will walk you through developing an Aggregator plugin, focusing on interface definitions and a case study.

## Aggregator Interface Definition

The primary role of an Aggregator plugin is to group individual Logs into LogGroups, which are then passed to the next-level flusher plugin for processing.

- **Add** interface for external input of Logs
- **Flush** interface for retrieving aggregated LogGroups
- **Reset** interface, currently internal and can be ignored
- **Init** interface, similar to the Input plugin's Init, returns a flush interval (in milliseconds) and an error flag. Unlike Input plugins, Aggregator's Init adds a `LogGroupQueue` type, defined in `loggroup_queue.go`:

```go
type LogGroupQueue interface {
    // Returns errAggAdd immediately if the queue is full.
    Add(loggroup *LogGroup) error
    // Wait up to @duration if the queue is full and return errAggAdd if the timeout occurs.
    // Avoid using this method if unsure.
    AddWithWait(loggroup *LogGroup, duration time.Duration) error
}
```

This interface represents an instance acting as a queue, where Aggregator instances can immediately submit aggregated LogGroup objects using the AddXXX methods. It provides a higher-realtime link, allowing Aggregator to submit new aggregations immediately after they are generated, without waiting for a Flush call.

However, note that Add will return an error when the queue is full, typically indicating a flusher blockage, such as network issues.

```go
// Aggregator is the interface for implementing an Aggregator plugin.
// RunningAggregator wraps this interface and ensures that Add, Push, and Reset cannot be called concurrently, so no locking is needed.

type Aggregator interface {
    // Init initializes system resources, like sockets or mutexes.
    // Returns the flush interval (in ms) and an error flag.
    // If the interval is 0, use the default interval.
    Init(Context, LogGroupQueue) (int, error)

    // Description provides a one-sentence description of the Input.
    Description() string

    // Add the metric to the aggregator.
    Add(log *protocol.Log) error

    // Flush pushes the current aggregates to the accumulator.
    Flush() []*protocol.LogGroup

    // Reset resets aggregator caches and aggregates.
    Reset()
}
```

## Developing an Aggregator Plugin

The development process for an Aggregator plugin involves the following steps:

1. Create an issue to discuss the plugin's functionality, and if the community approves, proceed to step 2.
2. Implement the Aggregator interface, using a sample pattern. See [aggregator/defaultone](https://github.com/alibaba/ilogtail/blob/main/plugins/aggregator/defaultone/aggregator_default.go) for a detailed example.
3. Register the plugin in the [Aggregators](https://github.com/alibaba/ilogtail/blob/main/plugin.go) by implementing the plugin_type in the JSON configuration, starting with "aggregator_". See [aggregator/defaultone](https://github.com/alibaba/ilogtail/blob/main/plugins/aggregator/defaultone/aggregator_default.go) for an example.
4. Add the plugin to the [plugin reference configuration file](https://github.com/alibaba/ilogtail/blob/main/plugins.yml) in the `common` section. If it's specific to a system, add it to the `linux` or `windows` section.
5. Perform unit tests or E2E testing, following [unit test instructions](../test/unit-test.md) and [E2E test instructions](../test/e2e-test.md).
6. Run `make lint` to check code style.
7. Submit a Pull Request.
