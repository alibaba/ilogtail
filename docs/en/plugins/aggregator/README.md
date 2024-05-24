# Aggregation

The aggregation plugin is used to bundle events for increased sending efficiency. It is applicable in the following scenarios:

* You are using the [SLS Output Plugin](../flusher/flusher-sls.md).
* You are using both the [SLS Output Plugin](../flusher/flusher-sls.md) and the [Extended Processing Plugin](../processor/README.md).

In cases where users do not specify an aggregation plugin in their collection configuration, `iLogtail` will default to the [Context Aggregation Plugin](./aggregator-context.md).
