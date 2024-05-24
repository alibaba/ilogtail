# Conditional Field Processing

## Introduction

The `processor_fields_with_condition` plugin enables dynamic field expansion or deletion based on the value of specific log field(s). [Source code](https://github.com/alibaba/ilogtail/blob/main/plugins/processor/fieldswithcondition/processor_fields_with_condition.go)

## Version

[Stable](../stability-level.md)

### Condition Evaluation

* Supports multiple field value comparisons.
* Supported operators: equals (default), regexp, contains, startswith.
* Logical operators: and (default), or.

### Filtering Capabilities

* By default, only performs dynamic field handling.
* Filtering can be enabled, discarding logs if no conditions are met.

### Processing Capabilities

* Follows a similar usage pattern to existing plugins like `processor_add_fields` and `processor_drop`.
* Supports multiple condition cases for dynamic field processing, but stops after the first match is found.

## Configuration Parameters

### `processor_fields_with_condition` Configuration

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type    | String, required, no default | Plugin type, always set to `processor_fields_with_condition`      |
| DropIfNotMatchCondition | Boolean, `false` | Discard logs if no matching condition is found (true) or keep them (false). |
| Switch | Array, each value is a Condition, no default | Switch actions based on conditions. |

### `Condition` Type Details

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Case | ConditionCase, required, no default | Conditions that the log data must meet. |
| Actions | Array, each value is a ConditionAction, required, no default | Actions to execute when the condition is met. |

### `ConditionCase` Type Details

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| LogicalOperator | String, `and` | Logical operator for multiple field conditions (and/or). |
| RelationOperator | String, `equals` | Operator for comparing field values (equals/regexp/contains/startwith). |
| FieldConditions | Map, no default, keys and values are Strings | Map of field names and expressions. |

### `ConditionAction` Type Details

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| type | String, required, no default | Action type, can be `processor_add_fields` or `processor_drop`. |
| IgnoreIfExist | Boolean, `false` | Whether to ignore the action if the same key already exists. |
| Fields | Map, no default, keys and values are Strings | Map of fields to add or update. |
| DropKeys | Array, no default, values are Strings | Fields to drop. |

## Example

* Collector Configuration (using the `metric_mock` plugin to simulate input)

```yaml
enable: true
inputs:
  - Type: metric_mock
    Fields:
      content : "{\"t1\": \"2022-07-22 12:50:08.571218122\", \"t2\": \"2022-07-22 12:50:08\", \"a\":\"b\",\"c\":2,\"d\":10, \"seq\": 20}"
      t1 : "2022-07-22 12:50:08.571218122"
      t2 : "2022-07-22 12:50:08"
      a : b
      c : "2"
      d : "10"
      seq : "20"
processors:
    - Type: processor_fields_with_condition
      DropIfNotMatchCondition: true 
      Switch:
        - Case:
            FieldConditions:
              seq: "10"
              d: "10"
            Actions:
              - type: processor_add_fields
                IgnoreIfExist: false
                Fields:
                  eventCode: event_00001
                  name: error_oom
              - type: processor_drop
                DropKeys:
                  - c
        - Case:
            FieldConditions:
              seq: "20"
              d: "10"
            Actions:
              - type: processor_add_fields
                IgnoreIfExist: false
                Fields:
                  eventCode: event_00002
                  name: error_oom2
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
  "Index": "1",
  "a": "b",
  "c": "2",
  "content": "{\"t1\": \"2022-07-22 12:50:08.571218122\", \"t2\": \"2022-07-22 12:50:08\", \"a\":\"b\",\"c\":2,\"d\":10, \"seq\": 20}",
  "d": "10",
  "seq": "20",
  "t1": "2022-07-22 12:50:08.571218122",
  "t2": "2022-07-22 12:50:08",
  "eventCode": "event_00002",
  "name": "error_oom2",
  "__time__": "1658490721"
}
```
