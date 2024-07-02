# Unit Testing

## Test Framework

If you need to include testing commands like `assert`, `require`, `mock`, and `suite`, you can use the `github.com/stretchr/testify` package. For more information, refer to the [Usage Guide](https://github.com/stretchr/testify).

## Testing Tools

The `ilogtail.Context` interface in the `ilogtail` package contains meta-configuration information for iLogtail, which allows for the creation of Mock Context and Mock Collector for unit testing purposes.

```go
import (
    "github.com/stretchr/testify/assert"
    "github.com/alibaba/ilogtail/pkg/pipeline"
    "github.com/alibaba/ilogtail/plugins/test"
    "github.com/alibaba/ilogtail/plugins/test/mock"
    "testing"
)

func TestInputSystem_CollectOpenFD(t *testing.T) {
    cxt := mock.NewEmptyContext("project", "store", "config")
    p := pipeline.MetricInputs["metric_system_v2"]().(*InputSystem)
    _, err := p.Init(cxt)
    assert.NoError(t, err, "cannot init the mock process plugin: %v", err)
    c := &test.MockMetricCollector{}
    p.CollectOpenFD(c)
    assert.Equal(t, 2, len(c.Logs))
    m := make(map[string]string)
    for _, log := range c.Logs {
        for _, content := range log.Contents {
            if content.Key == "__name__" {
                m[content.Value] = "exist"
            }
        }
    }
    assert.NotEmpty(t, m["fd_allocated"])
    assert.NotEmpty(t, m["fd_max"])
}
```

## Testing Plugin Behavior

To verify plugin behavior through logging, refer to the [Advanced Logging Functionality](../plugin-development/logger-api.md) guide.
