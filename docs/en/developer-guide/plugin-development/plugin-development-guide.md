# Open-Source Plugin Development Guide

## Understanding the ilogtail Plugin

For an in-depth explanation of ilogtail's plugin implementation, architecture, and system design, please refer to the [Plugin System](../../principle/plugin-system.md) documentation.

## Development Workflow

The ilogtail plugin development process typically involves the following steps:

1. **Create an Issue**: Describe the desired plugin functionality. The community will discuss its feasibility, and if approved, proceed to step 2.

2. **Implement required interfaces**.

3. **Register the plugin via the init function**.

4. **Add the plugin to the [plugin reference configuration file](https://github.com/alibaba/ilogtail/blob/main/plugins.yml)**. Place it in the `common` section for universal use, or in `linux` or `windows` sections for specific platforms.

5. **Write unit tests or E2E tests**; refer to [Writing Unit Tests](../test/unit-test.md) and [E2E Testing](../test/e2e-test.md) for guidance.

6. **Lint the code** with `make lint`.

7. **Submit a Pull Request**.

During development, the [Checkpoint API](./checkpoint-api.md) and [Logger API](./logger-api.md) can be helpful. You can also use the [Pure Plugin Mode](./pure-plugin-start.md) to test your plugin with minimal overhead.

For more detailed development steps, see:

* [Developing Input Plugins](./how-to-write-input-plugins.md)
* [Developing Processor Plugins](./how-to-write-processor-plugins.md)
* [Developing Aggregator Plugins](./how-to-write-aggregator-plugins.md)
* [Developing Flusher Plugins](./how-to-write-flusher-plugins.md)
* [Developing Extension Plugins](./how-to-write-extension-plugins.md)

## Documentation Writing Workflow

Once development is complete, you can generate plugin documentation using [How to Generate Plugin Docs](./how-to-genernate-plugin-docs.md). Alternatively, you can write the documentation manually.

The documentation process typically includes:

1. **Follow the [plugin documentation template](./plugin-doc-templete.md)** to write the plugin documentation.

2. **Add the plugin information** to the [Data Pipeline Overview](https://github.com/Takuka0311/ilogtail/blob/doc/docs/cn/plugins/overview.md). Arrange plugins alphabetically by their English names, ensuring the correct order.

3. **Include the plugin documentation path** in the [Document Index](https://github.com/Takuka0311/ilogtail/blob/doc/docs/cn/SUMMARY.md), maintaining the same order as the overview.

---

This translation maintains the original Markdown structure and content. If you need further assistance or have specific requirements, please let me know.
