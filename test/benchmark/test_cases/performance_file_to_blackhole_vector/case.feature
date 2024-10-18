@input
Feature: performance file to blackhole vector
  Performance file to blackhole vector

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeVector
    Given {docker-compose} environment
    Given docker-compose boot type {benchmark}
    When start docker-compose {performance_file_to_blackhole_vector}
    When start monitor {vector}
    When generate random json logs to file, speed {100}MB/s, total {1}min, to file {./a.log}
    When stop monitor in {30} seconds and verify if log processing is finished
