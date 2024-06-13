# E2E测试

iLogtail提供了一个完整的E2E测试引擎，方便您快速开展集成测试，从而进一步保证代码质量。在大部分情况下，您只需要编写一个配置文件来定义测试行为，即可轻松完成测试。

## 工作原理

E2E测试采用行为驱动开发（Behavior-Driven Development）的设计思路，通过定义一系列测试行为，并通过配置文件的方式来描述测试场景，从而实现对插件的集成测试。测试引擎会根据配置文件中的内容，根据正则匹配对应的函数，并解析配置文件中的参数，传递给对应的函数。从而完成自动创建测试环境、启动iLogtail、触发日志生成、验证日志内容等一系列操作，最终输出测试报告。
相关参考：
- [https://cucumber.io/docs/bdd/](https://cucumber.io/docs/bdd/)
- [https://github.com/cucumber/godog](https://github.com/cucumber/godog)

## 测试流程

### 环境准备

在准备开始进行集成测试前，您首先需要准备以下内容：

- 测试环境：主机（可通过SSH访问）、K8s集群（可通过kubeconfig访问）
- 部署 iLogtail

### 配置文件

对于每一个新的功能，您都需要在./test/cases/core/scenarios目录下创建一个新的feature配置文件。每个配置文件中可以包含多个测试场景，每个测试场景由一个或多个步骤组成。

配置文件的基本框架如下所示：

```plain
@processor
Feature: processor regex
  Test processor regex

  @e2e @host
  Scenario: TestRegexSingle
    Given {host} environment
    Given {regex_single} config as below
    """
  enable: true
  inputs:
    - Type: input_file
      FilePaths:
        - /tmp/ilogtail/regex_single.log
  processors:
    - Type: processor_parse_regex_native
      SourceKey: content
      Regex: (\S+)\s(\w+):(\d+)\s(\S+)\s-\s\[([^]]+)]\s"(\w+)\s(\S+)\s([^"]+)"\s(\d+)\s(\d+)\s"([^"]+)"\s(.*)
      Keys:
        - mark
        - file
        - logNo
        - ip
        - time
        - method
        - url
        - http
        - status
        - size
        - userAgent
        - msg
    """
    When generate {100} regex logs, with interval {100}ms
    Then there is {100} logs
    Then the log contents match regex single
```

- `Feature`定义了一个测试功能，下面为这个功能的描述信息。在`Feature`下可以定义多个测试场景。
- `Scenario`定义了一个测试场景。在`Scenario`下可以定义多个行为。
- 行为定义分为三类：
  - `Given`：定义了一些准备测试条件的行为。
  - `When`：定义了一些触发测试条件的行为。
  - `Then`：定义了一些验证测试条件的行为。
- 行为中使用`{}`作为标识符，该部分内容将作为参数，传递给对应的Go函数。
- `@`表示一个tag，在运行测试时，会根据tag的不同，分别运行。除了自定义的tag外，测试框架定义了一些默认的tag：
  - `@e2e`：表示该测试场景为E2E测试。
  - `@regression`：表示该测试场景为回归测试。
  - `@host`：表示该测试场景在host环境下运行。
  - `@k8s`：表示该测试场景在k8s环境下运行。

### 运行测试

在所有测试内容准备完毕后，您可以直接在test目录下以go test的方式运行E2E测试。根据您的需求，可以选择运行所有测试或者指定测试。

```shell
go test -v -run ^TestE2EOnK8s$ github.com/alibaba/ilogtail/test/cases/core
go test -v -run ^TestE2EOnHost$ github.com/alibaba/ilogtail/test/cases/core
```

### 添加行为

#### 1. 编写行为函数
测试框架中已经定义了一系列的行为函数，您可以直接使用这些函数来完成测试。具体定义可以参考`testhub/README.md`文件。

如果您需要添加新的行为函数，可以在`testhub`目录下添加一个Go函数。不同目录下的行为函数的职责有所不同：
- `cleanup`：清理测试环境，其中的测试函数会默认在测试结束后执行。无需在配置文件中显式声明使用。
- `control`：管控相关的行为函数，如初始化环境、添加配置等。
- `setup`：初始化测试环境，并提供远程调用的相关功能。
- `trigger`：数据生成相关的行为函数，如生成日志等。
- `verify`：验证相关的行为函数，如验证日志数量、验证日志内容等。

每个行为函数的接口定义如下所示：

```go
func LogCount(ctx context.Context, expect int) (context.Context, error) {
    // your code here
}
```

函数的第一个参数必须为`context.Context`。除此之外，后续可添加任意多个参数。函数的返回值为`context.Context`和`error`，其中`context.Context`为传递给下一个行为函数的参数，`error`为错误信息。

#### 2. 注册行为函数

在`test/cases/core/main_test.go`中，您需要注册您的行为函数。注册函数的格式如下所示：

```go
func scenarioInitializer(ctx *godog.ScenarioContext) {
	// Given

	// When

	// Then
	ctx.Then(`^there is \{(\d+)\} logs$`, verify.LogCount)
}
```

您需要根据行为的类型，将行为函数注册到对应的位置。在`Given`中注册`setup`中的行为函数，在`When`中注册`trigger`中的行为函数，在`Then`中注册`verify`中的行为函数。`control`中的行为函数比较灵活，根据函数职责不同，注册到不同的类型中。

在注册时，您需要定义一个正则表达式，用于匹配配置文件中的行为。在正则表达式中，您可以使用`{}`来标识参数，这些参数将会传递给行为函数。例如：

```go
ctx.Then(`^there is \{(\d+)\} logs$`, verify.LogCount)
```

能够从配置文件中匹配到`there is {100} logs`这样的行为，并将`100`作为参数传递给`LogCount`函数。

#### 3. 在配置文件中使用

在配置文件中，您可以直接使用您定义的行为函数。例如：

```plain
Then there is {100} logs
```

在运行测试时，测试框架会根据配置文件中的行为，调用对应的行为函数，并传递参数。
