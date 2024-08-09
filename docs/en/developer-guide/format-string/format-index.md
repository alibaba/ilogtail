# Elasticsearch Dynamic Index Formatting

When developing plugins to write data into Elasticsearch, users often have dynamic index requirements, which differ slightly from Kafka's dynamic topics.

Common scenarios for dynamic indices include:

- **Field-based indexing:** Similar to Kafka topics, create indices based on specific business fields.
- **Time-based indexing:** Index by day, week, or month, as seen in the ELK community, e.g., `%{+yyyy.MM.dd}` for daily, `%{+yyyy.ww}` for weekly.

Combining these needs, the rules for creating dynamic indices are:

- `%{content.fieldname}`: Retrieve the value of a field from the `contents` object.
- `%{tag.fieldname}`: Fetch a value from the `tags` object, like `%{tag.k8s.namespace.name}`.
- `${env_name}`: Directly obtain from environment variables, supported since version `1.5.0`.
- `%{+yyyy.MM.dd}`: Create by day, note the `+` before the time format is essential.
- `%{+yyyy.ww}`: Create by week, and other time formats are similar. Golang's time format might not be user-friendly, so refer to the ELK community's conventions for expressions.

The function for formatting dynamic indices looks like this:

```go
func FormatIndex(targetValues map[string]string, indexPattern string, indexTimestamp uint32) (*string, error)
```

- `targetValues` is a map that uses the Convert protocol to handle key-value pairs to be replaced. You can refer to the usage of `ToByteStreamWithSelectedFields` in `flusher_kafka_v2`.
- `indexPattern` is the dynamic index expression.
- `indexTimestamp` is of the same type as the `time` field in the `Log` protobuf, allowing you to use the timestamp from the log for dynamic index formatting.
