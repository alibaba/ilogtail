# Developing Extensions

Extensions provide custom capabilities and interface implementations that can be referenced by other plugins (input, processor, aggregator, flusher) in a pipeline.

## Extension Interface Definition

The role of an Extension is to offer a generic way to register specific capabilities (usually implementations of specific interfaces). Once registered, other pipeline plugins can depend on and cast the Extension to the desired interface.

- **Description**: A one-sentence description of the Extension.
- **Init**: The initialization interface for the plugin, which might involve setting up system resources, like sockets or mutexes.
- **Stop**: Stopping the plugin, such as disconnecting from external systems.

```go
type Extension interface {
    // Description returns a one-sentence description of the Extension.
    Description() string

    // Init is called for initializing system resources, like sockets or mutexes.
    Init(Context) error

    // Stop stops the services and releases resources.
    Stop() error
}
```

## Extension Development Workflow

The development of an Extension follows these steps:

1. Create an issue to describe the desired plugin functionality. Community members will discuss the feasibility of the plugin development. If approved, proceed to step 2.

2. Implement the **Extension** interface (note: in addition to the generic Extension interface, implement the specific interface methods for the desired Extension's capabilities, like [extension/basicauth](https://github.com/alibaba/ilogtail/blob/main/plugins/extension/basicauth/basicauth.go) implements [extensions.ClientAuthenticator](https://github.com/alibaba/ilogtail/blob/main/pkg/pipeline/extensions/authenticator.go)).

3. Register the Extension in the [Extensions](https://github.com/alibaba/ilogtail/blob/main/plugin.go) using the `init` function. The plugin's registration name (the `plugin_type` in the JSON configuration) must start with "ext_". See [extension/basicauth](https://github.com/alibaba/ilogtail/blob/main/plugins/extension/basicauth/basicauth.go) for an example.

4. Add the plugin to the `common` section of the [plugin configuration file](https://github.com/alibaba/ilogtail/blob/main/plugins.yml). If the plugin is intended for a specific system, add it to the `linux` or `windows` section.

5. Write unit tests or E2E tests, following the instructions in [Writing Unit Tests](../test/unit-test.md) and [E2E Testing](../test/e2e-test.md).

6. Run `make lint` to check code style.

7. Submit a Pull Request.
