@input
Feature: performance file to blackhole filebeat
  Performance file to blackhole filebeat

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeFilebeat
    Given {docker-compose} environment
    Given docker-compose boot type {benchmark}
    When start docker-compose {performance_file_to_blackhole_filebeat}
    When start monitor {filebeat}
<<<<<<< HEAD
    When generate logs to file, speed {100}MB/s, total {1}min, to file {./a.log}, template
    """
    {"url": "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1", "ip": "10.200.98.220", "user-agent": "aliyun-sdk-java", "request": {"status": "200", "latency": "18204"}, "time": "07/Jul/2022:10:30:28"}
    """
=======
    When generate random json logs to file, speed {10}MB/s, total {1}min, to file {./a.log}
    When wait monitor until log processing finished
>>>>>>> benchmark
