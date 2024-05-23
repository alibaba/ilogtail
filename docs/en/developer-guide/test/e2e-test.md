# E2E test

iLogtail provides a complete E2E test engine to facilitate the integration testing of various plugins to further ensure code quality. In most cases, you only need to write a Yaml configuration file to define the test behavior to easily complete the test. If the test process involves external environment dependencies,You only need to provide an additional image of the environment (or the files needed to build the image), which saves the trouble of manually configuring the Test environment (including the network).

## Test process

### Environment preparation

Before preparing for the integration test, you must prepare the following in the development environment:

- docker engine
- docker-compose v2: make sure that 'docker-compose -- version' is run on the command line to output the results normally, and the container name containing ilogtailC and goc does not exist in the non-Test related containers during the test.
- goc

### Directory organization

For each test scenario, you must create a new folder in the./test/case/behavior directory and place the files required for the test in the folder, which must contain a test engine configuration file named ilogtail-e2e.yaml. So farWe have provided some test cases. You only need to add them on this basis. The directory organization format is as follows:

```plain
./test/case/
├── behavior
│   ├── <your_test_case_name>
│   │   ├── ilogtail-e2e.yaml
│   │   ├── docker-compose.yaml (optional, required when external environment dependencies are involved)
│   │   ├── Dockerfile (optional, required when external environment dependencies are involved and the dependency does not have an existing image)
│   │   └── other files required for testing (such as code or scripts required for image generation)
│   └── other test cases
├── core
└── performance
```

### Configuration file

The basic framework of the Yaml configuration file is as follows. The 'boot' module specifies how to create a test environment. Currently, only Docker compose are supported:

```yaml
boot:
# [Required] test environment configuration
    category: docker-compose# the Test environment is created by docker-compose by default.
testing_interval: <string># [required] the duration of a single verification. The timeout period will automatically stop.
ilogtail:
# [Required] iLogtail configuration
trigger:
# [Optional] trigger configuration
subscriber:
# [Optional] subscriber configuration
verify:
# [Required] validator configuration
retry:
# [Optional] verify the retry configuration
```

The following describes the configurations of required and some optional modules. The other optional configurations are described in the following scenarios.

#### iLogtail configuration

The configuration items related to iLogtail are as follows:

```yaml
ilogtail:
  config:
    - name: <string>        # configuration name
      detail:               # configuration content
        map<string, object>
  close_wait: <string>      # The time to wait for data transmission after the test.
  env:                      # ilogtail the environment variable of the container.
    map<string, string>
  depends_on:               # iLogtail the dependent container of the container in the same format as the docker-compose file.
    map<string, object>
  mounts:                   # iLogtail the directory Mount status of the container.
    array<string>
```

#### Validator configuration

The configuration items related to the validator are as follows:

```yaml
verify:
  log_rules:
# Business log verification configuration
    - name: <string>         # The name of the validation item.
      validator: <string>    # validator name
      spec:                  # verify the Device Configuration
        map<string, object>
    - ...
  system_rules:
    # System log verification configuration
    - name: <string>         # The name of the validation item.
      validator: <string>    # validator name
      spec:                  # verify the Device Configuration
        map<string, object>
    - ...
retry:
  times: <int>               # the maximum number of retries after the verification fails.
  interval: <string>         # retry interval
```

For more information about optional validators, see [Test Engine plugin Overview](https://github.com/alibaba/ilogtail/blob/main/test/docs/plugin-list.md).

### Run the test

After all the test contents are ready, you can run 'test_scope =<your_test_case_name> make e2e' directly in the root directory of the iLogtail '. If you need the detailed process of the engine output test, you can add 'test_debug = true' to the preceding command '.

After the test is completed, a new directory named behavior_test is created under the root directory, which covers files related to the test process. The directory structure is as follows:

```plain
./behavior_test/
├── report
│   ├── <your_test_case_name>_log
│       ├── ilogtail.LOG
│       └── logtail_plugin.LOG 
│   ├── <your_test_case_name>_coverage.out  (code coverage result)
│   ├── <your_test_case_name>_engine.log    (test engine log)
│   └── <your_test_case_name>_report.json   (test report)
├── ilogtail-test-tool
└── plugin_logger.xml
```

## Test mode

Depending on the plugin type to be tested, the test engine provides different components to facilitate you to generate the required logs or verify the log Content. The following describes the typical test methods for each plugin type.

### Input plugin test

For input plugins, most cases have external dependencies, so integration tests must be conducted. To do this, you need to provide images of log generation units (that is, input sources) (such as mysql and nginx) and define them in docker-compose.yaml. If the dependency does not have a ready-made image,You need to provide additional Dockerfile to build the image and other files used to build the image.

If you need to trigger the log generation unit (for example, the nginx running log),E2E test engine provides a trigger component that regularly sends http requests to the destination. You can configure the component in the engine configuration file.

In this scenario, the iLogtail uses the grpc protocol by default to send the data received from the input directly to the log subscriber, and then the subscriber sends the data to the validator for verification. The overall test architecture is shown in the following figure:

![inputs-test-framework](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/test-framework/inputs-test-framework.jpg)

The internal structure of the iLogtail container is shown in the following figure:

![inputs-ilogtail](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/test-framework/inputs-ilogtail.jpg)

Typical configuration examples for input plugin testing are as follows:

```yaml
boot:
  category: docker-compose
testing_interval: <string>
ilogtail:
  config:
    - name: <string>
      detail:
        - inputs:
            - Type: <string>    
            # Enter the specific configuration of the plugin.
trigger:
  url: <string>                 # http request destination address
  method: <string>              # http request method
  interval: <string>            # http request interval
  times: <int>                  # http requests
verify:
  log_rules:
    - name: <string>
      validator: <string>
      spec: 
        map<string, object>
    - ...
  system_rules:
    - name: <string>
      validator: <string>
      spec: 
        map<string, object>
    - ...
retry:
  times: <int>
  interval: <string>
```

Currently, we have provided a large number of test cases for input plugins. For more information, see the sample beginning with input_in the [https://github.com/alibaba/ilogtail/tree/main/test/ case/behavior] folder.

### Handle plugin testing

Because external dependencies are not required, it is not necessary to handle the integration test of plugins, but you recommend also complete the test. To complete the test, iLogtail provides the data simulation input plugins 'metric_mock 'and 'service_mock', which can directly generate the required data in the ilogtail without external input.

The same as the input plugin test scenario, in this scenario, the iLogtail uses the grpc protocol by default to send the data received from the input directly to the log subscriber, and then the subscriber sends the data to the validator for verification. The overall test architecture is shown in the following figure:

![processors-test-framework](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/test-framework/processors-test-framework.jpg)

The internal structure of the iLogtail container is shown in the following figure:

![processors-ilogtail](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/test-framework/processors-ilogtail.jpg)

Typical configuration examples for handling plugin tests are as follows (take the 'metric_mock 'input plugin as an example):

```yaml
boot:
  category: docker-compose
testing_interval: <string>
ilogtail:
  config:
    - name: <string>
      detail:
        - inputs:
            - Type: metric_mock
              IntervalMs: <int>
              Tags:
                map<string, string>
              Fields:
                map<string, string>
          processors:
            - Type: <string>  # The processing plugin to be tested
# Handle the specific configuration of the plugin.
verify:
  log_rules:
    - name: <string>
      validator: <string>
      spec: 
        map<string, object>
    - ...
  system_rules:
    - name: <string>
      validator: <string>
      spec: 
        map<string, object>
    - ...
retry:
  times: <int>
  interval: <string>
```

### Output plugin test

For output plugins, most cases have external dependencies, so integration tests must be performed. To do this, you need to provide an image (such as kafka) of the log storage unit (that is, the destination where the iLogtail is written) and define it in the docker-compose.yaml.If the dependency does not have a ready-made image, you need to provide an additional Dockerfile (and other files used to build the image).

To complete the test, iLogtail provides the data simulation input plugins 'metric_mock 'and 'service_mock', which can directly generate the required data in the ilogtail without external input.
For the output part, because the iLogtail writes data directly to the target storage unit, the written data must be pulled from the test engine to verify the correctness of the data. Therefore, you must write a corresponding Subscriber Subscriber for your output plugin to pull data written by iLogtail in the log storage unit,Therefore, the next verification can be performed. For more information about how to write a custom subscriber, see [how to write a custom subscriber](./How-to-add-subscriber.md).

The following figure shows the test architecture of the output plugin:

![flushers-test-framework](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/test-framework/flushers-test-framework.jpg)

The internal structure of the iLogtail container is shown in the following figure:

![flushers-ilogtail](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/test-framework/flushers-ilogtail.jpg)

Typical configuration examples for output plugin testing are as follows:

```yaml
boot:
  category: docker-compose
testing_interval: <string>
ilogtail:
  config:
    - name: <string>
      detail:
        - inputs:
            - Type: metric_mock
              IntervalMs: <int>
              Tags:
                map<string, string>
              Fields:
                map<string, string>
          outputs:
            - Type: <string>    # output plugin to be tested
# Configure the output plugin
subscriber:
  name: <string>                # The name of the custom subscriber.
  config: <object>              # customize the configuration of the subscriber.
verify:
  log_rules:
    - name: <string>
      validator: <string>
      spec: 
        map<string, object>
    - ...
  system_rules:
    - name: <string>
      validator: <string>
      spec: 
        map<string, object>
    - ...
retry:
  times: <int>
  interval: <string>
```
