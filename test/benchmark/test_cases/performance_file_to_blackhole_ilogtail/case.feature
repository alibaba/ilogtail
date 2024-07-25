@input
Feature: performance file to blackhole iLogtail
  Performance file to blackhole iLogtail

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeiLogtail
    Given {docker-compose} environment
    Given docker-compose type {e2e}
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
    - Type: flusher_sls
      Region: cn-hangzhou
      Endpoint: cn-hangzhou.log.aliyuncs.com
      Project: test_project
      Logstore: test_logstore
    """
    Given iLogtail container mount {./a.log} to {/home/test-log/json.log}
    When start docker-compose {performance_file_to_blackhole_ilogtail}
    When start monitor {ilogtailC}
    When generate logs to file, speed {10}MB/s, total {1}min, to file {./a.log}, template
    """
    {"url": "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1", "ip": "10.200.98.220", "user-agent": "aliyun-sdk-java", "request": {"status": "200", "latency": "18204"}, "time": "07/Jul/2022:10:30:28"}
    """
