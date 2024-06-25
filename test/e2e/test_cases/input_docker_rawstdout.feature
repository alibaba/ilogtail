@input
Feature: input docker rawstdout
  Test input docker rawstdout

  @e2e @docker-compose
  Scenario: TestInputDockerRawStdout
    Given {docker-compose} environment
    Given subcribe data from {grpc} with config
    """
    """
    Given {input-docker-rawstdout-case} config as below
    """
    enable: true
    inputs:
      - Type: service_docker_stdout_raw
        IncludeEnv:
          STDOUT_SWITCH: "true"
    """
    When start docker-compose dependencies {input_docker_rawstdout}
    Then there is at least {1} logs
    Then the log fields match kv
    """
    _time_: ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)?([zZ]|([\+-])([01]\d|2[0-3]):?([0-5]\d)?)?$
    content: "^hello$"
    _source_: "^stdout$"
    _image_name_: "^e2e_container:latest$"
    _container_name_: "^e2e-container-1$"
    _container_ip_: ^\b(?:(?:2(?:[0-4][0-9]|5[0-5])|[0-1]?[0-9]?[0-9])\.){3}(?:(?:2([0-4][0-9]|5[0-5])|[0-1]?[0-9]?[0-9]))\b$
    """
  
  @e2e @docker-compose
  Scenario: TestInputDockerRawStdoutMultiline
    Given {docker-compose} environment
    Given subcribe data from {grpc} with config
    """
    """
    Given {input-docker-rawstdout-multiline-case} config as below
    """
    enable: true
    inputs:
      - Type: service_docker_stdout_raw
        IncludeEnv:
          STDOUT_SWITCH: "true"
        Stdout: true
        BeginLineRegex: "today"
        BeginLineCheckLength: 1
    """
    When start docker-compose dependencies {input_docker_rawstdout_multiline}
    Then there is at least {1} logs
    Then the log fields match kv
    """
    _time_: ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]+)?([zZ]|([\+-])([01]\d|2[0-3]):?([0-5]\d)?)?$
    content: "^\ntoday\nhello$"
    _source_: "^stdout$"
    _image_name_: "^e2e_container:latest$"
    _container_name_: "^e2e-container-1$"
    _container_ip_: ^\b(?:(?:2(?:[0-4][0-9]|5[0-5])|[0-1]?[0-9]?[0-9])\.){3}(?:(?:2([0-4][0-9]|5[0-5])|[0-1]?[0-9]?[0-9]))\b$
    """