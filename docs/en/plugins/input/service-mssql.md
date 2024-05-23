# MS Sql Server Import Plugin

## Introduction

The `service_mssql` `input` plugin can collect data from SQL Server queries.

## Stability Level

[Beta](../stability-level.md)

## Configuration Parameters

| Parameter | Type, Default Value | Description |
| --- | --- | --- |
| Type | String, Required (no default) | Plugin type, set to `service_mssql`. |
| Host | String, `127.0.0.1` | Database host. |
| Port | Integer, `1433` | Database port. |
| User | String, `root` | Database username. |
| Password | String, No default (required) | Database password. |
| Database | String, No default (required) | Database name. |
| DialTimeoutMs | Integer, `5000` | Database connection timeout in milliseconds, default is 5000ms. |
| ReadTimeoutMs | Integer, `5000` | Database query timeout in milliseconds, default is 5000ms. |
| Statement | String, No default (required if CheckPoint is true) | SQL statement. When CheckPoint is set to true, the WHERE condition in the Statement must include CheckPointColumn, and configure the column value as $1. For example: if CheckPointColumn is set to `id`, the Statement would be `SELECT * from ... where id > $1`. |
| Limit | Boolean, `false` | Whether to use LIMIT pagination. If not configured, the default is false, indicating no LIMIT pagination. It's recommended to use LIMIT for pagination. Setting Limit to true will automatically append a LIMIT statement to the SQL query. |
| PageSize | Integer, No default (required if Limit is true) | Pagination size. Must be configured if Limit is set to true. |
| MaxSyncSize | Integer, `0` | Maximum number of records to sync per call. Unconfigured by default, which means no limit. |
| CheckPoint | Boolean, `false` | Whether to use checkpoint. Unconfigured by default, which means no checkpoint. |
| CheckPointColumn | String, No default (required if CheckPoint is true) | Name of the checkpoint column. Must be configured if CheckPoint is set to true. Note: The column value must be monotonically increasing, otherwise data collection may miss records (the maximum value in each query result will be used as input for the next query). |
| CheckPointColumnType | String, No default (required if CheckPoint is true) | Type of the checkpoint column, supports `int` and `time`. `int` is internally stored as `int64`, while `time` supports MySQL's date, datetime, and time formats. Must be configured if CheckPoint is set to true. |
| CheckPointStart | String, No default (required if CheckPoint is true) | Initial checkpoint value. Must be configured if CheckPoint is set to true. |
| CheckPointSavePerPage | Boolean, No default | If set to true, save the checkpoint after each page; if set to false, save the checkpoint after each sync. |
| IntervalMs | Integer, No default | Sync interval in milliseconds. |

## Example

* Create a LogtailTest database and LogtailTestTable in the SQL Server database.

```sql
IF NOT EXISTS(SELECT * FROM sys.databases WHERE name = 'LogtailTest')
BEGIN
    CREATE DATABASE [LogtailTest]
END
GO

USE [LogtailTest]
GO

IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='LogtailTestTable' and xtype='U')
BEGIN
    CREATE TABLE LogtailTestTable (
        id INT PRIMARY KEY IDENTITY (1, 1),
        name NVARCHAR(50),
        quantity INT
    )
END
GO

INSERT INTO LogtailTestTable (name, quantity) values('banana', 1);
INSERT INTO LogtailTestTable (name, quantity) values('banana', 2);
INSERT INTO LogtailTestTable (name, quantity) values('banana', 3);
INSERT INTO LogtailTestTable (name, quantity) values('banana', 4);
SELECT * FROM LogtailTestTable;
GO
```

* Collection configuration

```yaml
enable: true
inputs:
  - Type: service_mssql
    Address: 127.0.0.1
    CheckPoint: true
    CheckPointColumn: id
    CheckPointColumnType: int
    CheckPointSavePerPage: true
    CheckPointStart: "0"
    Database: LogtailTest
    IntervalMs: 1000
    Limit: true
    MaxSyncSize: 100
    PageSize: 100
    User: sa
    Password: xxxxx
    Statement: "select * from LogtailTestTable where id > ? order by id"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{"id":"1","name":"banana","quantity":"1","__time__":"1661416452"}
{"id":"2","name":"banana","quantity":"2","__time__":"1661416452"}
{"id":"3","name":"banana","quantity":"3","__time__":"1661416452"}
{"id":"4","name":"banana","quantity":"4","__time__":"1661416452"}
```
