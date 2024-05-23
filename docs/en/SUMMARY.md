# Table of contents

## About <a href="#about" id="about"></a>

* [What is iLogtail?](README.md)
* [Brief history](about/brief-history.md)
* [Benefits](about/benefits.md)
* [License](about/license.md)
* [Comparison between Community Edition and Enterprise Edition](about/compare-editions.md)

## Community activities <a href="#events" id="event"></a>

* [OSPP 2024](events/summer-ospp-2024/README.md)
  * [iLogtail community project introduction](events/summer-ospp-2024/projects/README.md)
    * [iLogtail Data Throughput Performance Optimization](events/summer-ospp-2024/projects/ilogtail-io.md)
    * [ConfigServer Capability Upgrade + Experience Optimization (Full Stack)](events/summer-ospp-2024/projects/config-server.md)

## Installation <a href="#installation" id="installation"></a>

* [Quick start](installation/quick-start.md)
* [Docker usage](installation/start-with-container.md)
* [Kubernetes usage](installation/start-with-k8s.md)
* [Daemon](installation/daemon.md)
* [Release notes](installation/release-notes.md)
* [Release notes(1.x)](installation/release-notes-1.md)
* [Supported operating systems](installation/os.md)
* [Source code](installation/sources/README.md)
  * [Download](installation/sources/download.md)
  * [Build](installation/sources/build.md)
  * [Docker image](installation/sources/docker-image.md)
  * [Dependencies](installation/sources/dependencies.md)
* [Mirrors](installation/mirrors.md)

## Concepts <a href="#concepts" id="concepts"></a>

* [Key concepts](concepts/key-concepts.md)

## Configuration <a href="#configuration" id="configuration"></a>

* [Collection config](configuration/collection-config.md)
* [System config](configuration/system-config.md)
* [Logging](configuration/logging.md)

## Plugins <a href="#plugins" id="plugins"></a>

* [Overview](plugins/overview.md)
* [Version Management](plugins/stability-level.md)
* [Inputs](plugins/input/README.md)
  * [input-file](plugins/input/input-file.md)
  * [input-container-stdio](plugins/input/input-container-stdio.md)
  * [input-command](plugins/input/input-command.md)
  * [service-docker-stdout](plugins/input/service-docker-stdout.md)
  * [metric-debug-file](plugins/input/metric-debug-file.md)
  * [metric-input-example](plugins/input/metric-input-example.md)
  * [metric-meta-host](plugins/input/metric-meta-host.md)
  * [metric-mock](plugins/input/metric-mock.md)
  * [metric-observer](plugins/input/metric-observer.md)
  * [metric-system](plugins/input/metric-system.md)
  * [service-canal](plugins/input/service-canal.md)
  * [service-goprofile](plugins/input/service-goprofile.md)
  * [service-gpu](plugins/input/service-gpu.md)
  * [service-http-server](plugins/input/service-http-server.md)
  * [sevice-input-example](plugins/input/service-input-example.md)
  * [service-journal](plugins/input/service-journal.md)
  * [service-kafka](plugins/input/service-kafka.md)
  * [service-mock](plugins/input/service-mock.md)
  * [service-mssql](plugins/input/service-mssql.md)
  * [service-otlp](plugins/input/service-otlp.md)
  * [service-pgsql](plugins/input/service-pgsql.md)
  * [service-syslog](plugins/input/service-syslog.md)
* [Processors](plugins/processor/README.md)
  * [Native](plugins/processor/native/README.md)
    * [processor-parse-regex-native](plugins/processor/native/processor-parse-regex-native.md)
    * [processor-parse-delimiter-native](plugins/processor/native/processor-parse-delimiter-native.md)
    * [processor-parse-json-native](plugins/processor/native/processor-parse-json-native.md)
    * [processor-parse-timestamp-native](plugins/processor/native/processor-parse-timestamp-native.md)
    * [processor-filter-regex-native](plugins/processor/native/processor-filter-regex-native.md)
    * [processor-desensitize-native](plugins/processor/native/processor-desensitize-native.md)
  * [Extended](plugins/processor/extended/README.md)
    * [processor-add-fields](plugins/processor/extended/processor-add-fields.md)
    * [processor-cloudmeta](plugins/processor/extended/processor-cloudmeta.md)
    * [processor-default](plugins/processor/extended/processor-default.md)
    * [processor-desensitize](plugins/processor/extended/processor-desensitize.md)
    * [processor-drop](plugins/processor/extended/processor-drop.md)
    * [processor-encrypy](plugins/processor/extended/processor-encrypy.md)
    * [processor-fields-with-condition](plugins/processor/extended/processor-fields-with-condition.md)
    * [processor-filter-regex](plugins/processor/extended/processor-filter-regex.md)
    * [processor-gotime](plugins/processor/extended/processor-gotime.md)
    * [processor-grok](plugins/processor/extended/processor-grok.md)
    * [processor-json](plugins/processor/extended/processor-json.md)
    * [processor-log-to-sls-metric](plugins/processor/extended/processor-log-to-sls-metric.md)
    * [processor-regex](plugins/processor/extended/processor-regex.md)
    * [processor-rename](plugins/processor/extended/processor-rename.md)
    * [processor-split-delimiter](plugins/processor/extended/processor-delimiter.md)
    * [processor-split-key-value](plugins/processor/extended/processor-split-key-value.md)
    * [processor-split-log-regex](plugins/processor/extended/processor-split-log-regex.md)
    * [processor-string-replace](plugins/processor/extended/processor-string-replace.md)
* [Aggregators](plugins/aggregator/README.md)
  * [aggregator-base](plugins/aggregator/aggregator-base.md)
  * [aggregator-context](plugins/aggregator/aggregator-context.md)
  * [aggregator-content-value-group](plugins/aggregator/aggregator-content-value-group.md)
  * [aggregator-metadata-group](plugins/aggregator/aggregator-metadata-group.md)
* [Flushers](plugins/flusher/README.md)
  * [flusher-kafka(Deprecated)](plugins/flusher/flusher-kafka.md)
  * [flusher-kafka_v2](plugins/flusher/flusher-kafka_v2.md)
  * [flusher-clickhouse](plugins/flusher/flusher-clickhouse.md)
  * [flusher-elasticsearch](plugins/flusher/flusher-elasticsearch.md)
  * [flusher-sls](plugins/flusher/flusher-sls.md)
  * [flusher-stdout](plugins/flusher/flusher-stdout.md)
  * [flusher-otlp](plugins/flusher/flusher-otlp.md)
  * [flusher-pulsar](plugins/flusher/flusher-pulsar.md)
  * [flusher-http](plugins/flusher/flusher-http.md)
  * [flusher-loki](plugins/flusher/flusher-loki.md)

## Principle <a href="#principle" id="principle"></a>

* [File discovery](principle/file-discovery.md)
* [Plugin system](principle/plugin-system.md)

## Observability <a href="#observability" id="observability"></a>

* [Logs](observability/logs.md)

## Developer Guide <a href="#developer-guide" id="developer-guide"></a>

* [Development Environment](developer-guide/development-environment.md)
* [Protocol](developer-guide/log-protocol/README.md)
  * [Converter](developer-guide/log-protocol/converter.md)
  * [How to add new protocol](developer-guide/log-protocol/How-to-add-new-protocol.md)
  * [Protocol Spec](developer-guide/log-protocol)
    * [Sls Protocol](developer-guide/log-protocol/protocol-spec/sls.md)
    * [Single Protocol](developer-guide/log-protocol/protocol-spec/single.md)
* [Code Style](developer-guide/codestyle.md)
* [Data Model](developer-guide/data-model.md)
* [Plugin Development](developer-guide/plugin-development/README.md)
  * [Plugin Development Guide](docs/cn/developer-guide/plugin-development/plugin-development-guide.md)
  * [Checkpoint API](developer-guide/plugin-development/checkpoint-api.md)
  * [Logger API](developer-guide/plugin-development/logger-api.md)
  * [How to write input plugins](developer-guide/plugin-development/how-to-write-input-plugins.md)
  * [How to write processor plugins](developer-guide/plugin-development/how-to-write-processor-plugins.md)
  * [How to write aggregator plugins](developer-guide/plugin-development/how-to-write-aggregator-plugins.md)
  * [How to write flusher plugins](developer-guide/plugin-development/how-to-write-flusher-plugins.md)
  * [How to generate plugin docs](developer-guide/plugin-development/how-to-genernate-plugin-docs.md)
  * [Plugin Doc Template](docs/cn/developer-guide/plugin-development/plugin-doc-templete.md)
  * [Pure Plugin Mode](developer-guide/plugin-development/pure-plugin-start.md)
* [Test](developer-guide/test/README.md)
  * [Unit Test](developer-guide/test/unit-test.md)
  * [E2E Test](developer-guide/test/e2e-test.md)
* [Code Check](developer-guide/code-check/README.md)
  * [Check Code Style](developer-guide/code-check/check-codestyle.md)
  * [Check License](developer-guide/code-check/check-license.md)
  * [Check Dependency License](developer-guide/code-check/check-dependency-license.md)

## Contributing Guide <a href="#controbuting-guide" id="controbuting-guide"></a>

* [Contributing](contributing/CONTRIBUTING.md)
* [Developer](contributing/developer.md)
* [Achievement](contributing/achievement.md)

## Benchmark <a href="#benchmark" id="benchmark"></a>

* [Performance Comparison between iLogtail and Filebeat in container scenarios](benchmark/performance-compare-with-filebeat.md)

## ConfigServer <a href="#config-server" id="config-server"></a>

* [Quick Start](config-server/quick-start.md)
* [Protocol](config-server/communication-protocol.md)
* [Developer Guide](config-server/developer-guide.md)

## Awesome iLogtail <a href="#awesome-ilogtail" id="awesome-ilogtail"></a>

* [Approaching iLogtail Community Edition](awesome-ilogtail/ilogtail.md)
* [Getting Started](awesome-ilogtail/getting-started.md)
* [Developer Guide](awesome-ilogtail/developer-guide.md)
* [Use Cases](awesome-ilogtail/use-cases.md)
