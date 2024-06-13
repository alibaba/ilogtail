@input
Feature: container filter
  Test container filter

  @regression @k8s
  Scenario: TestContainerFilterByLabel
    Given {daemonset} environment
    When add k8s label {{"rel":"e2e","tag":"v3.0.1"}}
    Given {container_filter_label} config as below
    """
  enable: true
  inputs:
    - Type: input_file
      EnableContainerDiscovery: true
      FilePaths:
        - /tmp/ilogtail/regex_single.log
      ContainerFilters:
        IncludeK8sLabel:
          rel: e2e
          tag: v3.0.1
  processors: []
    """
    When generate {100} regex logs, with interval {100}ms
    Then there is {100} logs

    When remove k8s label {{"rel":"e2e","tag":"v3.0.1"}}
    When generate {100} regex logs, with interval {100}ms
    Then there is {0} logs