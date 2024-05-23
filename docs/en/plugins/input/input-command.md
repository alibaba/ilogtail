# input_command Plugin

## Introduction

The `input_command` plugin generates executable scripts on the agent machine by configuring script content, then executes them using the specified `cmdpath`. The plugin retrieves information from the script's `stdout` after execution for parsing and obtaining details about the script's output.

**Note:** iLogtail must be run as the root user to use this plugin. Please review the script content for potential risks.

## Stability Level

[Alpha](../stability-level.md)

## Configuration Parameters

### Base Parameters

| Parameter                  | Type       | Required | Description                                                                                                                  |
|---------------------|----------|------|----------------------------------------------------------------------------------------------------------------------------|
| Type                | String   | Yes    | Plugin type, set to `input_command`                                                                                           |
| ScriptType          | String   | Yes    | Specifies the type of script content, currently supported: bash, shell, python2, python3                                                  |
| User                | String   | Yes    | The username to run the command with; only non-root users (recommend using the minimum privilege, granting only necessary rwx permissions) |
| ScriptContent       | String   | Yes    | Script content, supports PlainText or Base64-encoded content, corresponds to the ContentEncoding field; max length is 512*1024 bytes |
| ContentEncoding     | String   | No     | Script content format, either PlainText (unencoded) or Base64-encoded; default: PlainText                                            |
| LineSplitSep        | String   | No     | Separator for script output lines; if empty, all output is treated as one line                                                                 |
| CmdPath             | String   | No     | Path to execute the script; if empty, the default path is used (e.g., /usr/bin/bash for bash, /usr/bin/sh for shell, etc.)         |
| TimeoutMilliSeconds | int      | No     | Timeout for script execution, in milliseconds; default is 3000ms                                                                 |
| IntervalMs          | int      | No     | Collection trigger frequency, also the script execution frequency, in milliseconds; default is 5000ms                          |
| Environments        | []string | No     | Additional environment variables; defaults to os.Environ() values, with any provided values appended to the base environment      |
| IgnoreError         | Bool     | No     | Whether to log errors when the plugin fails; defaults to false if not specified, meaning errors are not ignored                      |

### Generation Parameters

| Parameter         | Type     | Description                                             |
|------------|--------|---------------------------------------------------------|
| content    | String | Represents the output of the script                     |
| script_md5 | String | MD5 hash of the ScriptContent (脚本内容) for identifying the source of the generated log |

* Collector Configuration 1

```yaml
enable: true
inputs:
  - Type: input_command
    User: test
    ScriptType: shell
    ScriptContent:
      echo -e "test metric commond"
    Environments:
      - "DEBUG=true"
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
  "content": "test metric commond",
  "script_md5": "b3ebc535c2e6cead6e2e13b383907245",
  "__time__": "1689677617"
}
```

* Collector Configuration 2

```yaml
enable: true
inputs:
  - Type: input_command
    User: test
    ScriptType: python2
    ScriptContent: |
      print("test input_command 0")
      print("test input_command 1")
      print("test input_command 2")
      print("test input_command 3")
      print("test input_command 4")
      print("test input_command 5")
      print("test input_command 6")
      print("test input_command 7")
      print("test input_command 8")
      print("test input_command 9")
    CmdPath: /usr/bin/python
    Environments:
      - "DEBUG=true"
    TimeoutMilliseconds: 1005
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
  "content": "test input_command 0\ntest input_command 1\ntest input_command 2\ntest input_command 3\ntest input_command 4\ntest input_command 5\ntest input_command 6\ntest input_command 7\ntest input_command 8\ntest input_command 9",
  "script_md5": "5d049f5b2943d2d5e5737dddb065c39c",
  "__time__": "1689677656"
}
```

* Collector Configuration 3

```yaml
enable: true
inputs:
  - Type: input_command
    User: test
    ScriptType: python2
    ScriptContent: |
      import os
      print(os.environ)
    CmdPath: /usr/bin/python
    Environments:
      - "DEBUG=true"
    TimeoutMilliseconds: 1005
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
```

* Output

```json
{
  "content": "{'GOPATH': '/opt/go', 'GOROOT': '/usr/local/go', 'DEBUG': 'true', ... (omitted rest of the environment variables)}",
  "script_md5": "1444286e16118f7d1bb778738a828273",
  "__time__": "1689677681"
}
```
