# Logger Interface

<https://github.com/alibaba/ilogtail/blob/main/pkg/logger/logger.go>

The Logger provides four levels of logging: DBG, INFO, WARN, and ERR, each with both key-value (kv) and formatted (format) print interfaces.

```go
// kv form
func Debug(ctx context.Context, kvPairs ...interface{})
func Info(ctx context.Context, kvPairs ...interface{})
func Warning(ctx context.Context, alarmType string, kvPairs ...interface{})
func Error(ctx context.Context, alarmType string, kvPairs ...interface{})

// format form
func Debugf(ctx context.Context, format string, params ...interface{})
func Infof(ctx context.Context, format string, params ...interface{})
func Warningf(ctx context.Context, alarmType string, format string, params ...interface{})
func Errorf(ctx context.Context, alarmType string, format string, params ...interface{})
```

The `alarmType` parameter for WARN and ERR levels typically uses all uppercase XXX_ALARM.

Basic usage example:

```go
func (p *plugin) func1() {
    logger.Debug(p.context.GetRuntimeContext(), "foo", "bar")
    logger.Warningf(p.context.GetRuntimeContext(), "TEST_ALARM", "msg %s", "param ignored")
}
```

Output:

```plaintext
2021-08-24 18:20:02 [DBG] [logger_test.go:174] [func1] [mock-configname,mock-logstore] foo:bar
2021-08-24 18:20:02 [WARN] [logger_test.go:175] [func1] [mock-configname,mock-logstore] AlarmType:TEST_ALARM msg param ignored
```

## Logging Metadata Information

For iLogtail, which supports multiple tenants, it can handle multiple collection configurations simultaneously. iLogtail allows printing metadata information from the collection configuration to the log for issue troubleshooting and localization.

```go
import (
    "github.com/alibaba/ilogtail/pkg/logger"
)
```

The following code block is essential for a plugin, and to print logs, you only need to pass `p.context.GetRuntimeContext()` as the first argument to the logger package. The output will automatically append the configuration and logstore names:

```go
type plugin struct {
    context ilogtail.Context
}

func (p *plugin) func1() {
    logger.Debug(p.context.GetRuntimeContext(), "foo", "bar")
}
```

If config and logstore names are present in the context:

```plaintext
2021-08-24 18:20:02 [DBG] [logger_test.go:174] [func1] [mock-configname,mock-logstore] foo:bar
```

If config and logstore names are not present in the context:

```plaintext
2021-08-24 18:20:02 [DBG] [logger_test.go:174] [func1] foo:bar
```

## Testing with Logger

### General Usage

For most cases, importing `_ "github.com/alibaba/ilogtail/pkg/logger/test"` is sufficient.

```go
import (
    "context"
    "testing"

    "github.com/alibaba/ilogtail/pkg/logger"
    _ "github.com/alibaba/ilogtail/pkg/logger/test"
)

func Test_plugin_func1(t *testing.T) {
    logger.Info(context.Background(), "foo", "bar")
}
```

### Advanced Usage - Custom Logger

You can use the `logger.ConfigOption` to customize the behavior of the logger, such as output, log level, and asynchronous printing.

```go
package test

import (
    "context"
    "testing"

    "github.com/alibaba/ilogtail/pkg/logger"
)

func init() {
    logger.InitTestLogger(logger.OptionDebugLevel)
}

func Test_plugin_func1(t *testing.T) {
    logger.Debug(context.Background(), "foo", "bar")
}
```

### Advanced Usage - Reading Log Content

In some test scenarios, developers may need to read log content to verify branch coverage. In such cases, you can use the `logger.OptionOpenMemoryReceiver` parameter.

```go
import (
    "context"
    "strings"
    "testing"

    "github.com/alibaba/ilogtail/pkg/logger"
    "github.com/stretchr/testify/assert"
)

func init() {
    logger.InitTestLogger(logger.OptionOpenMemoryReceiver)
}

func Test_plugin_func1(t *testing.T) {
    logger.ClearMemoryLog()
    logger.Info(context.Background(), "foo", "bar")
    assert.Equal(t, 1, logger.GetMemoryLogCount())
    assert.Truef(t, strings.Contains(logger.ReadMemoryLog(1), "foo:bar"), "got %s", logger.ReadMemoryLog(1))
}
```

### Controlling Log Behavior at Startup

By default, iLogtail logs asynchronously at the INFO level to a file. To dynamically adjust this behavior, refer to the following:

### Adjusting Log Level

If no `plugin_logger.xml` file exists in the directory relative to the starting program, you can use the following command to set the log level:

```shell
./ilogtail --logger-level=debug
```

If a `plugin_logger.xml` file exists, you can modify the file or use the following command to force a new log configuration file generation:

```shell
./ilogtail --logger-level=info --logger-retain=false
```

### Enabling Console Logging

By default, console logging is disabled in generated environments. To enable console logs in a local development environment, if no `plugin_logger.xml` file exists, use the following command:

```shell
./ilogtail --logger-console=true
```

If a `plugin_logger.xml` file exists, you can modify the file or use the following command to force a new log configuration file generation:

```shell
./ilogtail --logger-console=true --logger-retain=false
```
