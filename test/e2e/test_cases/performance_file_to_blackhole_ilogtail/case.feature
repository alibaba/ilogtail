@input
Feature: performance file to blackhole iLogtail
  Performance file to blackhole iLogtail

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeiLogtail
    Given {docker-compose} environment
    Given subcribe data from {grpc} with config
    """
    """
    Given {performance-file-to-blackhole-ilogtail-case} local config as below
    """
    enable: true
    inputs:
      - Type: input_file
        FilePaths: 
          - /home/test-log/json.log
    processors:
      - Type: processor_parse_json_native
        SourceKey: content
      - Type: processor_filter_regex_native
        FilterKey:
          - user-agent
        FilterRegex:
          - ^no-agent$
    flushers:
      - Type: flusher_stdout
        OnlyStdout: true
        Tags: true
    """
    Given iLogtail container mount {./a.log} to {/home/test-log/json.log}
    When start docker-compose {performance_file_to_blackhole_ilogtail}
    When start monitor {e2e-ilogtailC-1}
    When generate {50} logs every {1}ms, total {1}min, to file {./a.log}, template
    """
    {"url": "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1", "ip": "10.200.98.220", "user-agent": "aliyun-sdk-java", "request": {"status": "200", "latency": "18204"}, "time": "07/Jul/2022:10:30:28"}
    """