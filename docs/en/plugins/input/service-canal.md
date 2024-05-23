# Canal

## Introduction

The 'service_canal ''input' plug-in can collect MySQL Binlog.

## Version

[Beta](../stability-level.md)

## Configure parameters

| Parameter | Type, default value | Description |
| --- | --- | --- |
| Type | String, no default value (required) | Plug-in Type, specified as 'service_canal '. |
| Host | String,'127.0.0.1' | Database Host. |
| Port | Interger,'3306' | Database Port. |
| User | String,'root' | Database username. |
| Password | String. The default value is null. | Database Password. |
| ServerID | Interger，125 | Logtail模拟Mysql Slave的ID。|
| IncludeTables | Array, no default value (required) | Contains the table name (including db, such as test_db.test_table), which can be configured as a regular expression. If a table does not meet any of the conditions in the IncludeTables, the table is not collected.If you want to collect all tables, set this parameter '.*\\..*'. If you want to complete the match, add ^ before and $After it, for example: ^ test_db\\.test_table $. |
| ExcludeTables | Array, no default value (required) | Ignored table names (including db, such as test_db.test_table), can be configured as regular expressions. If a table meets any of the conditions in the ExcludeTables, the table is not collected.If this parameter is not set, all tables are collected by default. |
| StartBinName | String,'"' | Binlog file name collected for the first time. If this parameter is not set, the collection starts from the current time point by default. To collect data from a specified location, you can view the current Binlog file and the file size offset,And set the StartBinName and StartBinlogPos to the corresponding values. |
| StartBinlogPos | Interger,'true' | Indicates whether to attach the global transaction ID. If this parameter is not set, the default value is true. If this parameter is set to false, the global transaction ID is not attached to the uploaded data. |
| EnableGTID | Boolean,'true' | Indicates whether to attach the global transaction ID. If this parameter is not set, the default value is true. If this parameter is set to false, the global transaction ID is not attached to the uploaded data. |
| EnableInsert | Boolean,'true' | Whether to collect data from insert events. If this parameter is not set, the default value is true. If this parameter is set to false, the insert event data will not be collected. |
| EnableUpdate | Boolean,'true' | Whether to collect data from update events. If this parameter is not set, the default value is true. If this parameter is set to false, the update event data is not collected. |
| EnableDelete | Boolean,'true' | Whether to collect data from the delete event. If this parameter is not set, the default value is true. If this parameter is set to false, the delete event data is not collected. |
| EnableDDL | Boolean,'false' | Whether to collect DDL(data definition language) event data. If this parameter is not set, the default value is false, indicating that DDL event data is not collected. This option does not support IncludeTables and ExcludeTables filtering.|
| Charset | String,'utf-8' | Encoding method. If this parameter is not set, the default value is utf-8. |
| TextToString | Boolean,'false' | Whether to convert text data to a string. If this parameter is not set, the default value is false, indicating that no conversion is performed. |
| PackValues | Boolean,'false' | Whether to package event data into JSON format. Default value: false. If this parameter is set to true, the Logtail packages the event data in JSON format into the data and old_data fields,old_data is only meaningful in the row_update event. Example: assume that the data table has three columns of data c1,c2, and c3, set to false. The row_insert event data contains three fields c1,c2, and c3, and set to true,c1,c2, and c3 are uniformly packaged into data fields with values '{ "c1": "** ", "c2 ":" **", "c3": "** "}'. |
| EnableEventMeta | Boolean,'false' | Indicates whether to collect the metadata of the event. The default value is false, indicating that the metadata is not collected. The metadata of the Binlog event includes event_time, event_log_position, event_size, and event_server_id.|

## Sample

Perform INSERT, UPDATE, and DELETE operations on SpecialAlarm tables in the user_info database, as shown in the following figure.

* Table structure

```sql
CREATE TABLE `titles` (
  `emp_no` int(11) NOT NULL,
  `title` varchar(50) NOT NULL,
  `from_date` date NOT NULL,
  `to_date` date DEFAULT NULL,
  PRIMARY KEY (`emp_no`,`title`,`from_date`),
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
```

* Table content, one row

```text
emp_no title from_date to_date
10006 Senior Engineer 1990-08-05 9999-01-01
```

* Execute three statements

```sql
update titles set title = 'test-update' where emp_no = 10006

delete from titles where emp_no = 10006

INSERT INTO `titles` (`emp_no`, `title`, `from_date`, `to_date`)
VALUES
 (10006, 'Senior Engineer', '1990-08-05', '9999-01-01'); 
```

* Collection configuration

```yaml
enable: true
inputs:
  - Type: service_canal
    Host: 127.0.0.1
    Port: 3306
ServerID: 123456.
    Password: xxxxx
    EnableDDL: true
    TextToString: true
flushers:
  - Type: flusher_kafka
    Brokers:
-Localhost: 9092
    Topic: access-log
```

* Output

```Json
{"Time":1657890330,"Contents":[{"Key":"_table_","Value":"titles"},{"Key":"_offset_","Value":"4308"},{"Key":"_old_emp_no","Value":"10006"},{"Key":"from_date","Value":"1990-08-05"},{"Key":"_host_","Value":"127.0.0.1"},{"Key":"_event_","Value":"row_update"},{"Key":"_id_","Value":"12"},{"Key":"_old_from_date","Value":"1990-08-05"},{"Key":"_gtid_","Value":"00000000-0000-0000-0000-000000000000:0"},{"Key":"_db_","Value":"employees"},{"Key":"_filename_","Value":"mysql-bin.000001"},{"Key":"_old_title","Value":"Senior Engineer"},{"Key":"_old_to_date","Value":"9999-01-01"},{"Key":"emp_no","Value":"10006"},{"Key":"title","Value":"test-update"},{"Key":"to_date","Value":"9999-01-01"}]}

{"Time":1657890333,"Contents":[{"Key":"_id_","Value":"13"},{"Key":"_filename_","Value":"mysql-bin.000001"},{"Key":"emp_no","Value":"10006"},{"Key":"title","Value":"test-update"},{"Key":"_db_","Value":"employees"},{"Key":"_table_","Value":"titles"},{"Key":"_event_","Value":"row_delete"},{"Key":"from_date","Value":"1990-08-05"},{"Key":"to_date","Value":"9999-01-01"},{"Key":"_host_","Value":"127.0.0.1 "},{" key ":" _ gtid _ "," value ":":: 0 "},{" key ":" _ offset _ "," value ":" 4660 "}]}

{"Time":1657890335,"Contents":[{"Key":"_offset_","Value":"4975"},{"Key":"emp_no","Value":"10006"},{"Key":"title","Value":"Senior Engineer"},{"Key":"from_date","Value":"1990-08-05"},{"Key":"_gtid_","Value":"00000000-0000-0000-0000-000000000000:0"},{"Key":"_filename_","Value":"mysql-bin.000001"},{"Key":"_table_","Value":"titles"},{"Key":"_event_","Value":"row_insert"},{"Key":"_id_","Value":"14"},{"Key":"to_date","Value":"9999-01-01"},{"Key":"_host_","Value":"127.0.0.1"},{"Key":"_db_","Value":"employees"}]}
```
