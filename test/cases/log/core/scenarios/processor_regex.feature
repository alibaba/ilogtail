@processor
Feature: processor regex
  Test processor regex

  @e2e @host
  Scenario: TestRegexSingle
    Given {host} environment
    Given {regex_single} config as below
    """
  enable: true
  inputs:
    - Type: input_file
      FilePaths:
        - /tmp/ilogtail/regex_single.log
  processors:
    - Type: processor_parse_regex_native
      SourceKey: content
      Regex: (\S+)\s(\w+):(\d+)\s(\S+)\s-\s\[([^]]+)]\s"(\w+)\s(\S+)\s([^"]+)"\s(\d+)\s(\d+)\s"([^"]+)"\s(.*)
      Keys:
        - mark
        - file
        - logNo
        - ip
        - time
        - method
        - url
        - http
        - status
        - size
        - userAgent
        - msg
    """
    When generate {100} regex logs, with interval {100}ms
    Then there is {100} logs
    Then the log contents match regex single
