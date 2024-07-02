# What is iLogtail

![icon](<.gitbook/assets/ilogtail-icon.png>)

iLogtail is a lightweight, high-performance, and automated configuration tool designed for observability scenarios. It is widely used within Alibaba and thousands of Alibaba Cloud customers for collecting telemetry data such as logs, traces, and metrics. You can deploy it on physical machines, virtual machines, or Kubernetes environments to gather monitoring data.

[![GitHub stars](https://camo.githubusercontent.com/674a26318ece2d770231086a733bebdbb174c15721f03714f5b79930574a800a/68747470733a2f2f696d672e736869656c64732e696f2f6769746875622f73746172732f616c69626162612f696c6f677461696c)](https://github.com/alibaba/ilogtail/stargazers) [![GitHub issues](https://camo.githubusercontent.com/4266ec67b48f666bc0d440f9d1399e4b56ffc4eca3af3764e062731be83b2873/68747470733a2f2f696d672e736869656c64732e696f2f6769746875622f6973737565732f616c69626162612f696c6f677461696c)](https://github.com/alibaba/ilogtail/issues) [![GitHub license](https://camo.githubusercontent.com/608afe55a7ca2ed062304f89208d3b929fddcbde8923cd09ef40edb2d2c3bf76/68747470733a2f2f696d672e736869656c64732e696f2f6769746875622f6c6963656e73652f616c69626162612f696c6f677461696c)](https://github.com/alibaba/ilogtail/blob/main/LICENSE)

## Key Advantages

* Supports collection of various logs, traces, and metrics, with exceptional compatibility for container and Kubernetes environments.

* Minimal resource consumption, outperforming similar telemetry agents by 5-20 times in terms of performance.

* High stability, validated in production at Alibaba and with millions of deployments, processing hundreds of petabytes of observable data daily.

* Plugin-based extensibility allows customization of data collection, processing, aggregation, and sending.

* Remote configuration management supported, with graphical, SDK, and K8s Operator options, making it easy to manage data collection for millions of machines.

* Advanced features like self-monitoring, traffic control, resource control, proactive alerts, and data collection statistics.

## Features

**iLogtail** supports collecting diverse telemetry data and transmitting it to various backends, such as the [Alibaba Cloud SLS Observability Platform](https://help.aliyun.com/product/28958.html). The supported data types include:

* Logs
  * Collecting static log files.
  * Gathering files dynamically in containerized environments.
  * Capturing Stdout in containerized environments.

* Traces
  * OpenTelemetry protocol.
  * Skywalking V2 protocol.
  * Skywalking V3 protocol.
  * ...

* Metrics
  * Node metrics.
  * Process metrics.
  * GPU metrics.
  * Nginx metrics.
  * Supports Prometheus metrics.
  * Supports Telegraf metrics.
  * ...

## Quick Start

Since a C++ build environment can be complex, iLogtail relies on Docker for compilation. To build iLogtail from source, execute the following command:

```bash
make
cp example/quick_start/* output
cd output
./ilogtail
# iLogtail is now collecting the 'output/simple.log' file and outputting to standard output.
```

For more details, refer to the [compilation guide](https://ilogtail.gitbook.io/ilogtail-docs/installation/sources/build).

## Roadmap

iLogtail's development roadmap: [Roadmap](https://github.com/alibaba/ilogtail/discussions/422)

## Contributing

* [Contributing Guidelines](./contributing/CONTRIBUTING.md)
* [Developer Guide](./contributing/developer.md)
* [Achievements](./contributing/achievement.md)

## Our Users

iLogtail is currently being used by students from companies like Alibaba Cloud, Stone Paper Docs, Tujia Travel, Xiaohongshu, Bytedance, Bilibili, Didi Chuxing.

## License

[Apache 2.0 License](https://github.com/alibaba/ilogtail/blob/main/LICENSE)

## Contact Us

You can report bugs, suggest improvements, or participate in discussions through:

* [GitHub Issues](https://github.com/alibaba/ilogtail/issues)
* [GitHub Discussions](https://github.com/alibaba/ilogtail/discussions)

Alternatively, you can reach us through:

* WeChat Official Account: Log Service
* Bilibili: [Alibaba Cloud SLS](https://space.bilibili.com/630680534?from=search\&seid=2845737427240690794\&spm\_id\_from=333.337.0.0)
* Zhihu: [iLogtail Community](https://www.zhihu.com/column/c_1533139823409270785)
* Join our community chat group:

![chatgroup](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/chatgroup/chatgroup_20240408.png)
