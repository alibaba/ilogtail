# How to Develop a Processor Plugin

Processor plugins enhance input data processing, and this guide will walk you through interface definition and a case study for creating a processor plugin.

## Processor Interface Definition

The Processor plugin interface is straightforward. In addition to the standard `Init` and `Description` methods, there's only one `ProcessLogs` interface, which accepts a `[]*Log` input and returns a `[]*Log` output. This allows Processor plugins to form a sequential processing chain among instances.

```go
// A Processor can also function as a filter

type Processor interface {
    // Init initializes system resources, like sockets or mutexes.
    Init(Context) error

    // Description provides a one-sentence description of the input.
    Description() string

    // Apply the filter to the given metric.
    ProcessLogs(logArray []*protocol.Log) []*protocol.Log
}
```

## Developing a Processor

The development process for a Processor involves the following steps:

1. Create an issue to describe the plugin's functionality, and the community will discuss its feasibility. If approved, proceed to step 2.

2. Implement the Processor interface. We'll use a sample pattern for explanation. See the [processor/addfields](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/addfields) example for a detailed sample, and you can test the plugin with the [example.json](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/addfields/example.json).

3. Register the plugin in the [Processors](https://github.com/alibaba/ilogtail/tree/main/plugin.go) using the `Init` method. The plugin's registration name (the `plugin_type` in the JSON configuration) must start with "processor_". See the [processor/addfields](https://github.com/alibaba/ilogtail/tree/main/plugins/processor/addfields/processor_add_fields.go) example for details.

4. Add the plugin to the `common` section of the [plugin reference configuration file](https://github.com/alibaba/ilogtail/tree/main/plugins.yml). If it's specific to a particular system, add it to the `linux` or `windows` section.

5. Write unit tests or E2E tests, following the [instructions for unit tests](../test/unit-test.md) and [E2E tests](../test/e2e-test.md).

6. Run `make lint` to check coding conventions.

7. Submit a Pull Request.

## Performance Requirements for Processor Admittance

* Baseline test case: Use 512 randomly generated characters as content and run the plugin logic in its entirety.

* Speed threshold: The processing speed must be at least 40,000 operations per second (ops/s).
