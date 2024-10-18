@input
Feature: performance file to blackhole filebeat
  Performance file to blackhole filebeat

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeFilebeat
    Given {docker-compose} environment
    Given docker-compose boot type {benchmark}
    When start docker-compose {performance_file_to_blackhole_filebeat}
    When start monitor {filebeat}
    When generate random json logs to file, speed {10}MB/s, total {1}min, to file {./a.log}
    When stop monitor in {30} seconds and verify if log processing is finished
