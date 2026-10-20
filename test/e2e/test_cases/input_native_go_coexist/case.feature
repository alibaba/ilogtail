@input
Feature: native input with Go processor/flusher
  A3 removed the native-input whitelist/blacklist gate. These scenarios
  prove the newly allowed combinations actually collect and reach a Go
  processor plus the default Go flusher_grpc (injected by the subscriber).

  Same-pipeline multi input_file is still rejected (FileServer keying,
  tracked in #2644). Concurrent collection is covered by two pipelines.

  @e2e @docker-compose
  Scenario: TestMultiInputFileConcurrent
    Given {docker-compose} environment
    Given subcribe data from {grpc} with config
    """
    """
    Given {input-file-multi-a} local config as below
    """
    enable: true
    inputs:
      - Type: input_file
        FilePaths:
          - /root/test/a.log
        FlushTimeoutSecs: 1
    processors:
      - Type: processor_add_fields
        Fields:
          e2e_file: "a"
    """
    Given {input-file-multi-b} local config as below
    """
    enable: true
    inputs:
      - Type: input_file
        FilePaths:
          - /root/test/b.log
        FlushTimeoutSecs: 1
    processors:
      - Type: processor_add_fields
        Fields:
          e2e_file: "b"
    """
    Given loongcollector container mount {./a.log} to {/root/test/a.log}
    Given loongcollector container mount {./b.log} to {/root/test/b.log}
    When start docker-compose {input_native_go_coexist}
    Then there is at least {1} logs with filter key {e2e_file} value {a}
    Then there is at least {1} logs with filter key {e2e_file} value {b}
    Then there is at least {1} logs with filter key {content} value {file-a-live=====}
    Then there is at least {1} logs with filter key {content} value {file-b-live=====}

  @e2e @docker-compose
  Scenario: TestHostMetaWithGoProcessorFlusher
    Given {docker-compose} environment
    Given subcribe data from {grpc} with config
    """
    """
    Given {input-host-meta-go-case} local config as below
    """
    enable: true
    inputs:
      - Type: input_host_meta
        Interval: 5
    processors:
      - Type: processor_add_fields
        Fields:
          e2e_go_processor: "1"
    """
    When start docker-compose {input_native_go_coexist}
    Then there is at least {1} logs
    Then there is at least {1} logs with filter key {e2e_go_processor} value {1}
    Then there is at least {1} logs with filter key {__entity_type__} value {acs.ecs.process}
