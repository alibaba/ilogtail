@flusher
Feature: flusher elasticsearch
  Test flusher elasticsearch

  @e2e @docker-compose @WIP
  Scenario: TestFlusherElasticSearch
    Given {docker-compose} environment
    Given subcribe data from {elasticsearch} with config
    """
    address: http://localhost:9200
    username: elastic
    password: BtpoRTeyjmC=ruTIUoNN
    index: default
    """
    Given {flusher-elasticsearch-case} config as below
    """
    enable: true
    inputs:
      - Type: metric_mock
        IntervalMs: 100
        Fields:
          Index: "default"
          Content: "hello"
    flushers:
      - Type: flusher_elasticsearch
        Index: default
        Addresses: ["http://elasticsearch:9200"]
        Authentication:
          PlainText:
            Username: elastic
            Password: BtpoRTeyjmC=ruTIUoNN
    """
    Given iLogtail depends on containers {["clickhouse"]}
    When start docker-compose dependencies {flusher_elasticsearch}
    Then there is at least {10} logs
    Then the log fields match kv
    """
    index: "default"
    content: "hello"
    """
  
