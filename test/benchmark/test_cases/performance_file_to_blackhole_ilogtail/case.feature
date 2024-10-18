@input
Feature: performance file to blackhole iLogtail
  Performance file to blackhole iLogtail

  @e2e-performance @docker-compose
  Scenario: PerformanceFileToBlackholeiLogtail
    Given {docker-compose} environment
    Given docker-compose boot type {benchmark}
    When start docker-compose {performance_file_to_blackhole_ilogtail}
    When start monitor {ilogtailC}
    When generate random json logs to file, speed {100}MB/s, total {1}min, to file {./a.log}
    When stop monitor in {30} seconds and verify if log processing is finished