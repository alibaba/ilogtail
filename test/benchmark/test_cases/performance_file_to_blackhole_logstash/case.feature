@input
Feature: performance file to blackhole logstash
  Performance file to blackhole logstash

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeLogstash
    Given {docker-compose} environment
    Given docker-compose boot type {benchmark}
    When start docker-compose {performance_file_to_blackhole_logstash}
    When start monitor {logstash}
    When generate random nginx logs to file, speed {10}MB/s, total {1}min, to file {./a.log}
    When wait monitor until log processing finished
