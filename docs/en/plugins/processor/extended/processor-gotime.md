# Gotime

## Version

[Stable](../stability-level.md)

## Configuration Parameters

| Parameter | Type | Required | Description |
| --- | --- | --- | --- |
| SourceKey | String | Yes | Original field name. |
| SourceFormat | String | Yes | Format of the original timestamp. |
| SourceLocation | Int | No | Timezone of the original timestamp. If empty, it defaults to the timezone of the iLogtail host or container. |
| DestKey | String | Yes | Target field for the parsed timestamp. |
| DestFormat | String | Yes | Format for the parsed timestamp. |
| DestLocation | Int | No | Timezone for the parsed timestamp. If empty, it defaults to the local timezone. |
| SetTime | Boolean | No | Whether to set the parsed timestamp as the log timestamp. True (default) sets it. False does not. |
| KeepSource | Boolean | No | Whether to retain the original field in the parsed log. True (default) retains it. False discards it. |
| NoKeyError | Boolean | No | Whether to report an error if the original log lacks the specified source field. True (default) reports an error. False does not. |
| AlarmIfFail | Boolean | No | Whether to report an error if the timestamp extraction fails. True (default) reports an error. False does not. |

**Note**: The `SourceFormat` and `DestFormat` should follow the [Go's time layout](https://pkg.go.dev/time#Layout). Go uses a rule-based approach to parse time formats, for example, "1" represents months and "15" represents hours. To avoid parsing failures, it's recommended to use Go's time origin, "2006-01-02 15:04:05", as a sample for `SourceFormat` and `DestFormat`.

Alternatively, use these standard time formats:

```go
const (
    ANSIC       = "Mon Jan _2 15:04:05 2006"
    UnixDate    = "Mon Jan _2 15:04:05 MST 2006"
    RubyDate    = "Mon Jan 02 15:04:05 -0700 2006"
    RFC822      = "02 Jan 06 15:04 MST"
    RFC822Z     = "02 Jan 06 15:04 -0700" // RFC822 with numeric zone
    RFC850      = "Monday, 02-Jan-06 15:04:05 MST"
    RFC1123     = "Mon, 02 Jan 2006 15:04:05 MST"
    RFC1123Z    = "Mon, 02 Jan 2006 15:04:05 -0700" // RFC1123 with numeric zone
    RFC3339     = "2006-01-02T15:04:05Z07:00"
    RFC3339Nano = "2006-01-02T15:04:05.999999999Z07:00"
    Kitchen     = "3:04PM"
    // Handy time stamps.
    Stamp      = "Jan _2 15:04:05"
    StampMilli = "Jan _2 15:04:05.000"
    StampMicro = "Jan _2 15:04:05.000000"
    StampNano  = "Jan _2 15:04:05.000000000"
    DateTime   = "2006-01-02 15:04:05"
    DateOnly   = "2006-01-02"
    TimeOnly   = "15:04:05"
)
```

## Example

Collect logs from the `/home/test-log/` directory, specifically the `simple.log` file, and apply the provided configuration options to extract log information.

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths:
      - /home/test-log/*.log
processors:
  - Type: processor_gotime
    SourceKey: "content"
    SourceFormat: "2006-01-02 15:04:05"
    SourceLocation: 8
    DestKey: "d_key"
    DestFormat: "2006/01/02 15:04:05"
    DestLocation: 9
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Input

```bash
echo "2006-01-02 15:04:05" >> simple.log
```

* Output

```json
{
    "content": "2006-01-02 15:04:05",
    "d_key": "2006/01/02 16:04:05"
}
```
