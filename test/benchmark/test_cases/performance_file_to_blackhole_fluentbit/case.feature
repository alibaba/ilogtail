@input
Feature: performance file to blackhole fluentbit
  Performance file to blackhole fluentbit

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeFluentbit
    Given {docker-compose} environment
    Given docker-compose type {benchmark}
    When start docker-compose {performance_file_to_blackhole_fluentbit}
    When start monitor {fluent-bit}
    When generate logs to file, speed {10}MB/s, total {1}min, to file {./a.log}, template
    """
    {"url": "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1", "ip": "10.200.98.220", "user-agent": "aliyun-sdk-java", "request": {"status": "200", "latency": "18204"}, "time": "07/Jul/2022:10:30:28"}
    """
