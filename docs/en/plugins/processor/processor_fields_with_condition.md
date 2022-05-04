# processor_fields_with_condition
## Description
Processor to match multiple conditions, and if one of the conditions is met, the corresponding action is performed.
## Config
|  field   |   type   |   description   | default value   |
| ---- | ---- | ---- | ---- |
|DropIfNotMatchCondition|bool|Optional. When the case condition is not met, whether the log is discarded (true) or retained (false)|false|
|Switch|[]fieldswithcondition.Condition|The switch-case conditions|null|