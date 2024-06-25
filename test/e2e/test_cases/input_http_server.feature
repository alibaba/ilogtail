@input
Feature: input http server
  Test input http server

  @e2e @docker-compose
  Scenario: TestInputHTTPServer
    Given {docker-compose} environment
    Given subcribe data from {grpc} with config
    """
    """
    Given {input-http-server-case} config as below
    """
    enable: true
    inputs:
      - Type: service_http_server
        Address: ":18086"
        Format: influxdb
        FieldsExtend: true
    processors:
      - Type: processor_anchor
        SourceKey: content
        NoAnchorError: true
        Anchors:
          - Start: ""
            Stop: ""
            FieldName: body
            FieldType: json
            ExpondJson: true
    """
    When start docker-compose dependencies {input_http_server}
    When generate {10} http logs, with interval {10}ms, url: {http://ilogtailC:18086/?db=mydb}, method: {POST}, body:
    """
    weather,city=hz value=32
    """
    Then there is at least {1} logs
    Then the log fields match kv
    """
    "__tag__:db": "mydb"
    "__name__": "weather"
    "__value__": "32"
    "__labels__": "city#\\$#hz"
    "__type__": "float"
    "__time_nano__": "\\d+"
    """