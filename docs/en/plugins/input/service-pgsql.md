# PostgreSQL Import Plugin

## Introduction

The `service_pgsql` `input` plugin can collect data from PostgreSQL query results.

## Version

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, Required, No Default (Mandatory) | Plugin type, specify as `service_pgsql`. |
| Host | String, `127.0.0.1` | Database host. |
| Port | Integer, `5432` | Database port. |
| User | String, `root` | Database username. |
| Password | String, No Default (Optional) | Database password. |
| Database | String, No Default (Optional) | Database name. |
| DialTimeoutMs | Integer, `5000` | Database connection timeout in milliseconds, default is 5000ms. |
| Statement | String, No Default (Optional) | SQL statement. When CheckPoint is set to true, the WHERE condition in the SQL statement must include CheckPointColumn, and configure the value of that column as $1. For example: if CheckPointColumn is set to `id`, the Statement would be `SELECT * from ... where id > $1`. |
| Limit | Boolean, `false` | Whether to use LIMIT for pagination. If not configured, the default is false, indicating no LIMIT usage. It's recommended to use LIMIT for pagination. Setting Limit to true will automatically append a LIMIT clause to the SQL query. |
| PageSize | Integer, No Default (Required if Limit is true) | Pagination size. Must be configured if Limit is set to true. |
| MaxSyncSize | Integer, `0` | Maximum number of records to sync per call. Unconfigured by default, which means no limit. |
| CheckPoint | Boolean, `false` | Whether to use checkpoint. Unconfigured by default, which means no checkpoint usage. |
| CheckPointColumn | String, No Default (Required if CheckPoint is true) | Column name for checkpoint. Must be configured if CheckPoint is set to true. Note that the column value must be monotonically increasing, otherwise data collection may be missed (the maximum value in each query result will be used as input for the next query). |
| CheckPointColumnType | String, No Default (Required if CheckPoint is true) | Checkpoint column type, supports `int` and `time` types. `int` is internally stored as `int64`, while `time` supports MySQL's date, datetime, and time formats. Must be configured if CheckPoint is set to true. |
| CheckPointStart | String, No Default (Required if CheckPoint is true) | Initial checkpoint value. Must be configured if CheckPoint is set to true. |
| CheckPointSavePerPage | Boolean, No Default | If set to true, save the checkpoint after each page; if set to false, save the checkpoint after each sync. |
| IntervalMs | Integer, No Default | Sync interval in milliseconds. |

## Example

Collect data from the `specialalarmtest` table in the postgres database.

* Table structure and data:

```sql
CREATE TABLE IF NOT EXISTS specialalarmtest (
    id BIGSERIAL NOT NULL,
    time TIMESTAMP NOT NULL,
    alarmtype varchar(64) NOT NULL,
    ip varchar(16) NOT NULL,
    COUNT INT NOT NULL,
    PRIMARY KEY (id)
);

INSERT INTO specialalarmtest (time, alarmtype, ip, count) VALUES (now(), 'NO_ALARM', '10.10.***.***', 0);
INSERT INTO specialalarmtest (time, alarmtype, ip, count) VALUES (now(), 'NO_ALARM', '10.10.***.***', 1);
INSERT INTO specialalarmtest (time, alarmtype, ip, count) VALUES (now(), 'NO_ALARM', '10.10.***.***', 2);
INSERT INTO specialalarmtest (time, alarmtype, ip, count) VALUES (now(), 'NO_ALARM', '10.10.***.***', 3);
```

* Collection configuration:

```yaml
enable: true
inputs:
  - Type: service_pgsql
    Address: 127.0.0.1
    CheckPoint: true
    CheckPointColumn: id
    CheckPointColumnType: int
    CheckPointSavePerPage: true
    CheckPointStart: "0"
    Database: postgres
    IntervalMs: 1000
    Limit: true
    MaxSyncSize: 100
    PageSize: 100
    User: postgres
    Password: xxxxx
    Statement: "select * from specialalarmtest where id > $1"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output:

```json
{
  "id": "1",
  "time": "2022-08-25T06:01:07.276901Z",
  "alarmtype": "NO_ALARM",
  "ip": "10.10.***.***",
  "count": "0",
  "__time__": "1661416452"
}
{
  "id": "2",
  "time": "2022-08-25T06:01:07.277624Z",
  "alarmtype": "NO_ALARM",
  "ip": "10.10.***.***",
  "count": "1",
  "__time__": "1661416452"
}
{
  "id": "3",
  "time": "2022-08-25T06:01:07.278072Z",
  "alarmtype": "NO_ALARM",
  "ip": "10.10.***.***",
  "count": "2",
  "__time__": "1661416452"
}
{
  "id": "4",
  "time": "2022-08-25T06:01:07.278494Z",
  "alarmtype": "NO_ALARM",
  "ip": "10.10.***.***",
  "count": "3",
  "__time__": "1661416452"
}
```
