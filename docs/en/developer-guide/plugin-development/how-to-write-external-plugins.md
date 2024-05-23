# Developing External Private Plugins

## Scenario

In some cases, you might want to develop your own private plugin that can still benefit from the community's continuous updates to iLogtail without forking the main version. iLogtail's external plugin development mechanism enables you to achieve this goal.

## Steps

Let's assume you want to create an input plugin named `service_example_plugin` in your repository at `my-repo.com/my_space/my_plugins`.

### 1. Initialize a Go Project

```shell
go mod init my-repo.com/my_space/my_plugins
```

### 2. Import the Plugin API Package

Execute the following command in the root directory of your repository to import the necessary package:

```shell
go get github.com/alibaba/ilogtail/pkg
```

### 3. Create the Plugin Directory

```shell
mkdir example_plugin
```

### 4. Write Plugin Code

Implement the `ServiceInputV1` and `ServiceInputV2` interfaces in the `example_plugin.go` file:

```shell
cd example_plugin
touch example_plugin.go
```

```go
package example_plugin

import (
    "time"

    "github.com/alibaba/ilogtail/pkg/models"
    "github.com/alibaba/ilogtail/pkg/pipeline"
    "github.com/alibaba/ilogtail/pkg/protocol"
)

type MyServiceInput struct {
    Interval time.Duration `json:"interval" mapstructure:"interval"`
    collector   pipeline.Collector
    collectorV2 pipeline.PipelineCollector
    stopCh      chan struct{}
}

func (m *MyServiceInput) StartService(context pipeline.PipelineContext) error {
    m.collectorV2 = context.Collector()
    return nil
}

func (m *MyServiceInput) Init(context pipeline.Context) (int, error) {
    return 0, nil
}

func (m *MyServiceInput) Description() string {
    return "custom service input"
}

func (m *MyServiceInput) Stop() error {
    return nil
}

func (m *MyServiceInput) Start(collector pipeline.Collector) error {
    m.collector = collector
    go m.loop()
    return nil
}

func (m *MyServiceInput) loop() {
    for {
        select {
        case <-m.stopCh:
            return
        case <-time.After(m.Interval):
            if m.collector != nil {
                m.collector.AddRawLog(&protocol.Log{
                    Time: uint32(time.Now().Unix()),
                    Contents: []*protocol.Log_Content{
                        {Key: "host", Value: "1.1.1.1"},
                    },
                })
            }
            if m.collectorV2 == nil {
                continue
            }
            m.collectorV2.Collect(&models.GroupInfo{},
                models.NewSingleValueMetric("test_metric",
                    models.MetricTypeCounter,
                    models.NewTagsWithMap(map[string]string{"hello": "world"}),
                    time.Now().UnixNano(),
                    1),
            )
        }
    }
}
```

### 5. Add an Initialization Function and Register the Plugin

Append the following function to `example_plugin.go`:

```go
func init() {
    pipeline.AddServiceCreator("service_example_plugin", func() pipeline.ServiceInput {
        return &MyServiceInput{
            Interval: 5 * time.Second,
            stopCh:   make(chan struct{}, 1),
        }
    })
}
```

The plugin registration relies on the `init` function mechanism.

### 6. Create a Go File to Import All Plugins (Optional)

If your repository contains multiple plugins and you want to provide a convenient way to import all of them at once, create a `all.go` file in the root directory with the following content:

```go
package my_plugins

import (
    _ "my-repo.com/my_space/my_plugins/example_plugin"
)
```

The plugin repository directory structure would then look like this:

```plain
.
├── all.go
├── example_plugin
│   └── example_plugin.go
├── go.mod
└── go.sum
```

### 7. Write a Plugin Configuration File

**This step is performed in the iLogtail main repository.**

Create a configuration file named `external_plugins.yml` in the root of the iLogtail repository with the following content:

```yaml
plugins:

  common:

    - gomod: my-repo.com/my_space/my_plugins latest
      import: my-repo.com/my_space/my_plugins/example_plugin
```

To import all plugins from your private repository (including the ones in `all.go`), use this content:

```yaml
plugins:

  common:

    - gomod: my-repo.com/my_space/my_plugins latest
      import: my-repo.com/my_space/my_plugins  // Remove "example_plugin" from the import line
```

Afterward, build the project:

```shell
make all
```

For more information on customizing which plugins are included in the default build, refer to [Customizing Built-in Plugins](how-to-custom-builtin-plugins.md).
