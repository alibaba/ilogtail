字段添加
---

该插件可添加指定的字段（支持多个）。

#### 参数说明

插件类型（type）为 `processor_sls_metric`。

| 参数                 | 类型     | 必选或可选 | 参数说明                                                                                                             |
|--------------------|--------|-------|------------------------------------------------------------------------------------------------------------------|
| MetricTimeKey      | String | 可选    | 指定要用作时间戳的字段。默认取`log.Time`。确保指定的字段是合法的、符合格式的纳秒时间戳。                                                                |
| MetricLabelKeys    | Array  | 可选    | labels字段的key列表，key需遵循正则表达式： `[a-zA-Z_][a-zA-Z0-9_]*`。如果原始字段中存在 `__labels__` 字段，该值将被追加到列表中。 Label的Value不能包含竖线（\|） |
| MetricValues       | Map    | 可选    | 时序字段名所使用的key与时序值使用的key的映射。name需遵循正则表达式：`[a-zA-Z_:][a-zA-Z0-9_:]*` ，时序值可以是float类型的字符串。                            |
| CustomMetricLabels | Map    | 可选    | 要追加的自定义标签。 key需遵循正则表达式： `[a-zA-Z_][a-zA-Z0-9_]*`，Value不能包含竖线（\|）                                                 |

#### 示例

以下是一个示例配置，展示了如何使用 `processor_sls_metric` 插件来处理数据：

- 原始数据

```json
{
	"labelA": "AAA",
	"labelB": "BBB",
	"labelC": "CCC",
	"nameA": "myname",
	"valueA": "1.0",
	"nameB": "myname2",
	"valueB": "2.0",
    "__time__": "1579134612000000004"
}
```

- 处理配置

```json
{
  "processors": [
    {
      "type": "processor_sls_metric",
      "detail": {
        "MetricTimeKey": "__time__",
        "MetricLabelKeys": ["labelA", "labelB", "labelC"],
        "MetricValues": {
          "nameA": "valueA",
          "nameB": "valueB"
        },
        "CustomMetricLabels": {
          "labelD": "CustomD"
        }
      }
    }
  ]
}
```

- 处理后的数据

```json
[
  {
    "__labels__":"labelA#$#AAA|labelB#$#BBB|labelC#$#CCC|labelD#$#CustomD",
    "__name__":"myname",
    "__value__":"1.0",
    "__time_nanos__":"1579134612000000004"
  },
  {
    "__labels__":"labelA#$#AAA|labelB#$#BBB|labelC#$#CCC|labelD#$#CustomD",
    "__name__":"myname2",
    "__value__":"2.0",
    "__time_nanos__":"1579134612000000004"
  }
]
```

在上述示例中，原始数据包含了一些标签和指标值。通过配置 `processor_sls_metric` 插件，我们指定了要使用的字段和标签，并生成了处理后的数据对象。处理后的数据对象包含了指定的标签、指标值和时间戳字段。

请根据您的实际需求和数据结构，调整配置和字段名称。