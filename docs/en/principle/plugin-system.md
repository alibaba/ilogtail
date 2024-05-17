# Plugin System

## Background

As an agent for log collection, iLogtail currently runs on over a million machines, serving millions of applications. To better integrate with the open-source ecosystem and meet users' diverse接入 requirements, we have introduced a plugin system to iLogtail. Currently, it supports various data sources, including HTTP, MySQL Query, and MySQL Binlog.

This article provides an overview of the iLogtail plugin system, including its implementation principle, structure, and design.

## Implementation Principle

We'll illustrate the iLogtail plugin system's implementation using the following flowchart:

![Plugin System Implementation Diagram](https://sls-opensource.oss-us-west-1.aliyuncs.com/ilogtail/ilogtail-adapter-cgo.png?versionId=CAEQNBiBgMCl8rPG7RciIDdlMTUwYWE3MTk0YzRkY2ViN2E3MjgxYjlmODQzNDQx)

To support the plugin system, we introduced two dynamic libraries, libPluginAdaptor and libPluginBase (referred to as "adaptor" and "base"). Their relationship with iLogtail is as follows:

- iLogtail dynamically depends on these libraries (not included in the binary), initializing with attempts to load them using dynamic library interfaces (like dlopen). This design ensures that if loading fails, only plugin functionality is unavailable, not affecting iLogtail's existing features.

- Adaptor acts as a middle layer, both iLogtail and base rely on it. iLogtail registers callbacks, which the adaptor records and exposes to base as interfaces. Key among these callbacks is SendPb, through which base can add data to iLogtail's send queue for further processing by LogHub.

- Base is the core of the plugin system, containing essential functionalities like data collection, processing, aggregation, and output (to iLogtail). Expanding the plugin system essentially means extending base.

The flowchart outlines the steps:

- During iLogtail initialization, it loads the adaptor and registers callbacks using the obtained symbols.

- iLogtail loads base, acquiring symbols, including the LoadConfig symbol used in this process. Base depends on the adaptor in the binary, so it loads it during this step.

- iLogtail fetches the user's configuration (plugin type) from LogHub.

- iLogtail passes the configuration to base using LoadConfig.

- Base applies the configuration, starts up, and fetches data from the specified data source.

- Base calls the interface provided by the adaptor to submit data.

- Adaptor directly passes the data from base to iLogtail through the callback.

- iLogtail adds the data to the send queue and sends it to LogHub.

## System Composition and Design

This section focuses on libPluginBase, detailing the overall architecture and some system design aspects.

### Overall Architecture

![Log Plugin System Architecture](https://sls-opensource.oss-us-west-1.aliyuncs.com/ilogtail/logtail-libPluginBase.png?versionId=CAEQMxiBgIDM6YCk6BciIDBjYmVkZjQ2Yjg5NzQwY2NhZjI4MmFmZDA2M2MwZTU2)

As shown, the plugin system currently consists of Input, Processor, Aggregator, and Flusher components. The relationship between these components for each configuration is as follows:

- Input: Acts as the input layer, supporting one input per configuration, which can operate concurrently, collecting data to pass to the processor.

- Processor: As the processing layer, filters input data, such as checking field requirements or modifying fields. Multiple processors can be configured per configuration, working in a serial manner, with the output of one processor serving as input for the next.

- Aggregator: Acts as the aggregation layer, grouping data as needed, like bundling data or calculating field statistics. Like input, multiple aggregators can be configured for different purposes. Aggregators can proactively pass processed data to the flusher when new data arrives or periodically flush at a set interval.

- Flusher: Acts as the output layer, sending data to the specified target, such as local files or iLogtail (via the adaptor's interface). Multiple flushers can be configured per configuration, allowing data to be sent to multiple targets simultaneously.

Additionally, the plugin system includes Checkpoint, Statistics, and Alarm components, providing functionality for checkpointing, statistics, and alerts. These are encapsulated in a context, which plugins can access by calling provided interfaces without worrying about implementation details. We'll cover these features later.

### System Design

Here, we'll briefly introduce key design elements of the plugin system:

- **Ease of Extension and Development**: Implemented in Go for simplicity and scalability.

- **Compatibility with iLogtail's Existing Structure**: Supports dynamic configuration updates and unifies Alarm/Statistics.

- **Generic Functionality for Plugins**: Provides a checkpoint feature.

#### Using Go Language

To achieve extensibility and ease of development, we chose Go as the language for the plugin system. Key factors considered were:

- **Concurrency Isolation**: In production, iLogtail may run multiple plugins to meet diverse collection needs. Isolation and concurrent handling among plugins are essential. Go's design for concurrent programs makes it straightforward to implement these requirements, allowing developers to focus on their logic without worrying about the system's overall structure.

- **Dynamic Loading**: Given iLogtail's large deployment, using dynamic loading to minimize impact on the main iLogtail was a consideration. Go's native support for dynamic libraries is well-suited for this.

- **Community Alignment**: Telegraf and Beats, two popular open-source agents, use Go. Using Go aligns the plugin system with the community, reducing the learning curve for developers.

- **Go Plugin**: We are keeping an eye on Go plugin developments. If the time is right, a fully dynamic extension using Go plugins would further enhance user support.

- **Ease of Development**: Go's development efficiency generally surpasses that of C++ used by iLogtail, making it more suitable for plugin development scenarios.

### Dynamic Configuration Updates

The Log Service supports updating collection configurations through the console, which iLogtail then applies dynamically. To maintain this capability, the plugin system, as part of the configuration, must support dynamic updates too.

In the plugin system, configurations are organized per instance, and each instance creates and maintains input, processor, etc. plugins based on the configuration details. To enable dynamic updates, libPluginBase provides HoldOn/Resume/LoadConfig interfaces:

1. iLogtail checks for configuration updates affecting the plugin system and calls HoldOn to pause the system if necessary.

2. When the system is paused, the plugin instances stop, saving necessary checkpoint data.

3. After HoldOn, iLogtail loads updated or new configurations using LoadConfig. This call creates new configuration instances, replacing the old ones. To prevent data loss, new instances transfer data from the replaced instances before resuming.

4. Once the update is complete, iLogtail calls Resume to resume the plugin system, completing the dynamic configuration update.

### Unified Statistics and Alarm

In iLogtail, Statistics and Alarm mechanisms are implemented to aid users in statistical analysis and issue troubleshooting. The plugin system also requires this functionality, with each plugin potentially generating its own statistics or alerts during operation. To unify these with iLogtail's existing mechanisms, we created two global configurations, one for receiving statistics and another for alarms, outside user-defined configurations. These global configurations have a similar structure to user configurations, with an internal input plugin reading data from channels and passing it through for processing and output.

![image.png](https://sls-opensource.oss-us-west-1.aliyuncs.com/ilogtail/logtail-uni-alarm.png?versionId=CAEQMxiBgMCwsYyk6BciIDZhZDY0OWQ0NTg1ZTQ1YWRhYWNhODRjMDc5NzM4MmJk)

As shown in the diagram, on the right are two separate global configuration instances, which correspond to the reception and output of Statistics and Alarm, respectively. They adopt the same structure as the user configuration, using a built-in input plugin to read data from the channel at the entry point. After intermediary processing, the data is passed to the flusher plugin to output to iLogtail. Any plugin in the user configuration instance on the left can output statistical or alarm data according to its own logic and send it to the global configuration instances on the right via a Go channel.

### Checkpoint

To ensure data integrity in log collection, checkpointing is essential, especially for plugins like MySQL Binlog that need to record the current binlog file name and offset. Therefore, we implemented a generic checkpoint feature in the plugin system, providing a context API for plugins to access.

```go
type Context interface {
    SaveCheckPoint(key string, value []byte) error
    GetCheckPoint(key string) (value []byte, exist bool)
    SaveCheckPointObject(key string, obj interface{}) error
    GetCheckPointObject(key string, obj interface{}) (exist bool, ...)

    // ...
}
```
