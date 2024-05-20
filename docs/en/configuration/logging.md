# Logs

iLogtail, developed primarily in C++ and complemented with Go plugins, generates two types of logs: the main program logs and the plugin logs.

## iLogtail Main Program Logs

### Log Configuration File

The log control file is `apsara_log_conf.json`, which is auto-generated when iLogtail starts for the first time.

### Log Levels

Available levels: `TRACE`, `DEBUG`, `INFO`, `WARNING`, `ERROR`, `FATAL`.

### Log Types

* **Runtime Logs:**
  * Log file: `ilogtail.LOG`
  * Log level configuration: `Loggers:/apsara/sls/ilogtail:AsyncFileSink`
  
* **Profile Logs:**
  * Log file: `snapshot/ilogtail_profile.LOG`
  * No log level configuration needed.

* **Status Logs:**
  * Log file: `snapshot/ilogtail_status.LOG`
  * No log level configuration needed.

**Full Configuration:**

```json
{
  "Loggers": {
    "/": {
      "AsyncFileSink": "WARNING"
    },
    "/apsara/sls/ilogtail": {
      "AsyncFileSink": "INFO"
    },
    "/apsara/sls/ilogtail/profile": {
      "AsyncFileSinkProfile": "INFO"
    },
    "/apsara/sls/ilogtail/status": {
      "AsyncFileSinkStatus": "INFO"
    }
  },
  "Sinks": {
    "AsyncFileSink": {
      "Compress": "Gzip",
      "LogFilePath": "${ilogtail_run_path}/ilogtail.LOG",
      "MaxDaysFromModify": 300,
      "MaxLogFileNum": 10,
      "MaxLogFileSize": 20000000,
      "Type": "AsyncFile"
    },
    "AsyncFileSinkProfile": {
      "Compress": "",
      "LogFilePath": "${ilogtail_run_path}/snapshot/ilogtail_profile.LOG",
      "MaxDaysFromModify": 1,
      "MaxLogFileNum": 61,
      "MaxLogFileSize": 1,
      "Type": "AsyncFile"
    },
    "AsyncFileSinkStatus": {
      "Compress": "",
      "LogFilePath": "${ilogtail_run_path}/snapshot/ilogtail_status.LOG",
      "MaxDaysFromModify": 1,
      "MaxLogFileNum": 61,
      "MaxLogFileSize": 1,
      "Type": "AsyncFile"
    }
  }
}
```

## iLogtail Plugin Logs

### Log File

`logtail_plugin.LOG`

### Log Configuration File

`plugin_logger.xml`

### Log Levels

Available levels: `trace`, `debug`, `info`, `warn`, `error`, `critical`.

### Log Level Configuration

Modify the `minlevel` field to set the desired log level.

**Full Configuration:**

```xml
<seelog type="asynctimer" asyncinterval="500000" minlevel="info">
  <outputs formatid="common">
    <rollingfile type="size" filename="${ilogtail_run_path}/logtail_plugin.LOG" maxsize="2097152" maxrolls="10"/>
  </outputs>
  <formats>
    <format id="common" format="%Date %Time [%LEV] [%File:%Line] [%FuncShort] %Msg%n" />
  </formats>
</seelog>
```
