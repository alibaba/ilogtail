# Text log

## Introduction

The 'iuput_file' plug-in can collect logs from text files. The collected log content is saved in the 'content' field of the event.

## Version

[Stable](../stability-level.md)

## Configure parameters

| **parameter** | **type** | **required** | **default** | **description** |
| --- | --- | --- | --- | --- |
| Type | string | Yes | / | The plug-in Type. Fixed to input\_file. |
| FilePaths | \[string\] | Yes | / | The list of log file paths to be collected (currently only one path is allowed). You can use the \* and \*\* wildcard characters in the path. The \*\* wildcard characters can only appear once and can only be used before the file name.  |
| MaxDirSearchDepth | int | No | 0 | The maximum directory depth where the \*\* wildcard matches the file path. Valid only if the \*\* wildcard exists in the log path. Valid values: 0 to 1000. |
| ExcludeFilePaths | \[string\] | No | Empty | File path blacklist. The path must be an absolute path. You can use the \* wildcard. |
| ExcludeFiles | \[string\] | No | Empty | The blacklist of the file name. You can use the \* wildcard. |
| ExcludeDirs | \[string\] | No | Empty | Directory blacklist. The path must be an absolute path. You can use the \* wildcard |
| FileEncoding | string | No | utf8 | File encoding format. Optional values include utf8 and gbk. |
| TailSizeKB | uint | No | 1024 | When the configuration takes effect for the first time, the size of the start collection position of the matching file from the end of the file. If the file size is smaller than this value, the collection starts from scratch. The value range is 0 to 10485760KB.  |
| Multiline | object | No | Null | Multiline aggregation option. For more information, see table 1. |
| EnableContainerDiscovery | bool | No | false | Whether to enable container discovery. This parameter is valid only when the Logtail is running in Daemonset mode and the path of the collected file is in the container.  |
| ContainerFilters | object | No | Empty | Container filtering options. The relationship between multiple options is and, which is valid only when the EnableContainerDiscovery value is true,For more information, see table 2. |
| ExternalK8sLabelTag | map | No | Empty | For containers deployed in the K8s environment, you need to add additional tags related to Pod tags to the logs. The key in the map is the Pod tag name,value is the tag name. For example, if 'app: k8s_label_app' is added to the map, if the pod contains the 'app = serviceA' tag, this information is added to the log as a tag, that is, the field '_\_ tag \_\_: k8s\_label\_app: serviceA is added;If the 'app' tag is not included, an empty field \_\_ tag \_\_: k8s\_label\_app |
| ExternalEnvTag | map | No | Empty | For containers deployed in the K8s environment, you need to add additional tags related to container environment variables to the logs. The key in the map is the environment variable name, and the value is the corresponding tag name.For example, if 'VERSION: env_version' is added to the map, when the container contains the environment variable 'VERSION = v1.0.0 ', this information is added to the log in the form of a tag, that is, the field '_\_ tag \_\_: env\_version: v1.?0.0; If the 'VERSION' environment variable is not included, an empty field \_\_ tag \_\_: env\_version |
| AppendingLogPositionMeta | bool | No | false | Whether to add the metadata of the file to which the log belongs, including the____tag___:_____inode_____field and the_____file\_offset_____field.  |
| FlushTimeoutSecs | uint | No | 5 | If the file does not have a new complete log after the specified time, the content in the currently read cache is output as a log. |
| AllowingIncludedByMultiConfigs | bool | No | false | Whether to allow the current configuration to collect files that match other configurations. |

* Table 1: multi-row aggregation options

| **parameter** | **type** | **required** | **default** | **description** |
| --- | --- | --- | --- | --- |
| Mode | string | No | custom | Multiline aggregation Mode. Optional values include custom and JSON. |
| StartPattern | string | When Multiline.Mode is set to custom, at least one regular expression is required. | Null | Line start regular expression. |
| ContinuePattern | string | | Null | Line continues the regular expression. |
| EndPattern | string | | Null | Regular expression at the end of the line. |
| UnmatchedContentTreatment | string | No | single_line | For processing unmatched log segments, the optional values are as follows:<ul><li>discard: discard </li><li>single_line: store each row of unmatched log segments in a separate event </li></ul> |

* Table 2: container filtering options

| **parameter** | **type** | **required** | **default** | **description** |
| --- | --- | --- | --- | --- |
| K8sNamespaceRegex | string | No | Empty | For containers deployed in the K8s environment, specify the namespace conditions to which the pods to be collected belong. If this parameter is not added, all containers are collected.Regular match is supported. |
| K8sPodRegex | string | No | Empty | For containers deployed in the K8s environment, specify the Pod name condition of the container to be collected. If this parameter is not added, all containers are collected. Regular match is supported. |
| IncludeK8sLabel | map | No | Empty | For containers deployed in the K8s environment, specify the label conditions of the pods to be collected. The relationship between multiple conditions is "or". If this parameter is not added, all containers are collected.Regular match is supported. The key in the map is the name of the Pod tag, and the value is the value of the Pod tag. The description is as follows:<ul><li> If the value in the map is empty, all pods with the key in the pod tag will be matched;</li><li> If the value in the map is not empty,Then:<ul></li><li> if the value starts with '^' and ends with '$', the corresponding pod will be matched when the tag name of the pod is key and the corresponding tag value can match the value regularly. </li><li> in other cases,If the pod label has a key as the label name and a value as the label value, the corresponding pod is matched. </li> </ul> </ul> |
| ExcludeK8sLabel | map | No | Empty | For containers deployed in the K8s environment, specify the label conditions to exclude the pods where the collection containers are located. The relationship between multiple conditions is "or". If this parameter is not added,All containers are collected. Regular match is supported. The key in the map is the name of the pod tag, and the value is the value of the pod tag. The description is as follows:<ul><li> if the value in the map is empty, all pods that contain the key in the pod tag will be matched;</li><li>如果map中的value不为空，则：<ul></li><li>若value以`^`开头并且以`$`结尾，则当pod标签中存在以key为标签名且对应标签值能正则匹配value的情况时，相应的pod会被匹配；</li><li>其他情况下，当pod标签中存在以key为标签名且以value为标签值的情况时，相应的pod会被匹配。</li></ul></ul>       |
| K8sContainerRegex | string | No | Empty | Specify the name of the container to be collected for containers deployed in the K8s environment. If this parameter is not added, all containers are collected. Regular match is supported.  |
| IncludeEnv | map | No | Null | Specifies the environment variable condition of the container to be collected. The relationship between multiple conditions is "or". If this parameter is not added, all containers are collected. Regular match is supported. The key in the map is the name of the environment variable,value is the value of the environment variable, which is described as follows:<ul><li> If the value in the map is empty, containers that contain the key in the container environment variable will be matched;</li><li> If the value in the map is not empty, then:<ul></li><li> If the value starts with '^' and ends with '$,When the environment variable of the container uses key as the environment variable name and the corresponding environment variable value can match the value regularly, the corresponding container will be matched;</li><li> in other cases, when the environment variable of the container uses key as the environment variable name and value as the environment variable value,The corresponding containers are matched. </li> </ul> </ul> |
| ExcludeEnv | map | No | Null | Specifies the environment variable conditions that need to exclude the collection container. The relationship between multiple conditions is "or". If this parameter is not added, all containers are collected. Regular match is supported. The key in the map is the name of the environment variable,value is the value of the environment variable, which is described as follows:<ul><li> If the value in the map is empty, containers that contain the key in the container environment variable will be matched;</li><li> If the value in the map is not empty, then:<ul></li><li> If the value starts with '^' and ends with '$,When the environment variable of the container uses key as the environment variable name and the corresponding environment variable value can match the value regularly, the corresponding container will be matched;</li><li> in other cases, when the environment variable of the container uses key as the environment variable name and value as the environment variable value,The corresponding containers are matched. </li> </ul> </ul> |
| IncludeContainerLabel | map | No | Empty | Specifies the tag condition of the container to be collected. The relationship between multiple conditions is "or". If this parameter is not added, it is empty by default, indicating that all containers are collected. Regular match is supported.The key in the map is the name of the container label, and the value is the value of the container label. The description is as follows:<ul><li> If the value in the map is empty, all containers whose key is included in the container label will be matched;</li><li> If the value in the map is not empty,Then:<ul></li><li> If the value starts with '^' and ends with '$', the corresponding container will be matched when the tag name of the container is key and the corresponding tag value can match the value regularly. </li><li> in other cases, when the tag name of a container is key and the tag value is value,The corresponding containers are matched. </li> </ul> </ul> |
| ExcludeContainerLabel | map | No | Empty | Specifies the tag conditions that need to exclude collection containers. The relationship between multiple conditions is "or". If this parameter is not added, it is empty by default, indicating that all containers are collected.Regular match is supported. The key in the map is the name of the container label, and the value is the value of the container label. The description is as follows:<ul><li> If the value in the map is empty, all containers whose key is included in the container label will be matched;</li><li> If the value in the map is not empty,Then:<ul></li><li> if the value starts with '^' and ends with '$', the corresponding container will be matched when the tag name of the container is key and the corresponding tag value can match the value regularly. </li><li> in other cases, when the tag name of a container is key and the tag value is value,The corresponding containers are matched. </li> </ul> </ul> |

## Sample

### Collect files in a specified directory

Collect all files whose file names match the '*.log' rule in the'/home/test-log' path, and output the results to stdout.

* Input

```Json
{"key1": 123456, "key2": "abcd"}
```

* Collection configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* Output

```Json
{
    "__tag__:__path__": "/home/test-log/json.log",
    "content": "{\"key1\": 123456, \"key2\": \"abcd\"}",
    "__time__": "1657354763"
}
```

Note: the output of the '\_\_tag__' field varies depending on the version of the ilogtail. In order to accurately observe '\_\_tag__' in the standard output, it is recommended to carefully check the following points:
In the configuration of-fleful_stdout, 'Tags: true' is set'
-If a newer version of the ilogtail is used, when observing the standard output, '\_\_tag__' may be split into a separate line of information, prior to the output of the log content (this is different from the sample output in the document), please be careful not to observe the omission.

This note applies to all the places where the '\_\_tag__' field output is observed later.

### Collect all files in the specified directory

Collect all files (including recursion) whose file names match the *.log' rule in the '/home/test-log' path, and output the results to stdout.

* Collection configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/**/*.log
    MaxDirSearchDepth: 10
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

### Collect K8s container files (only iLogtail deployed in Daemonset)

Collect all the containers whose Pod name is prefixed with 'deploy' in the 'default' namespace, whose Pod label contains 'version: 1.0 'and the container environment variable is not 'ID = 123', and all the file names in the '/home/test-log/' path match '*.Log' rule file and output the result to stdout.

* Collection configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/*.log
    EnableContainerDiscovery: true
    ContainerFilters:
      K8sNamespaceRegex: default
      K8sPodRegex: ^(deploy.*)$
      IncludeK8sLabel:
        version: v1.0
      ExcludeEnv:
        ID: 123
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

### Collect simple multi-line logs

Collect the file '/home/test-log/regMulti.log'. The file content is split into logs by regular expression at the beginning of the row. Then, the log content is parsed by regular expression, fields are extracted, and the results are output to stdout.

* Input

```plain
[2022-07-07T10:43:27.360266763] [INFO] java.lang.Exception: exception happened
    at com.aliyun.sls.devops.logGenerator.type.RegexMultiLog.f2(RegexMultiLog.java:108)
    at java.base/java.lang.Thread.run(Thread.java:833)
```

* Collection configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/regMulti.log
    Multiline:
Start pattern: \[\ d +-\ d +-\ W: \ D: \ d +.\ D] \ s \[\ w] \ s.*
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Keys:
      - time
      - level
      - msg
Regex: \[(\ S +)]\ s \[(\ S +)]\ s(.*)
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* Output

```Json
{
    "__tag__:__path__": "/home/test-log/regMulti.log",
    "time": "2022-07-07T10:43:27.360266763",
    "level": "INFO",
    "msg": "java.lang.Exception: exception happened
    at com.aliyun.sls.devops.logGenerator.type.RegexMultiLog.f2(RegexMultiLog.java:108)
    at java.base/java.lang.Thread.run(Thread.java:833)",
    "__time__": "1657161807"
}
```

### Collect complex multi-line logs

Collect the file '/home/test-log/regMulti.log'. The file content is split into logs by regular expressions at the beginning and end of the line. Then, the log content is parsed by regular expressions, fields are extracted, and the results are output to stdout.

* Input

```plain
[2022-07-07T10:43:27.360266763] [ERROR] java.lang.Exception: exception happened
[2022-07-07T10:43:27.360266763]    at com.aliyun.sls.devops.logGenerator.type.RegexMultiLog.f2(RegexMultiLog.(Java:108)
[2022-07-07T10:43:27.360266763]    at java.base/java.lang.Thread.run(Thread.java:833)
[2022-07-07T10:43:27.360266763]    ... 23 more
[2022-07-07T10:43:27.360266763] Some user custom log
[2022-07-07T10:43:27.360266763] Some user custom log
[2022-07-07T10:43:27.360266763] [ERROR] java.lang.Exception: exception happened
```

* Collection configuration

```yaml
enable: true
inputs:
  - Type: input_file
    FilePaths: 
      - /home/test-log/regMulti.log
    Multiline:
Startpatern: \[\ d-\ d +-\ w +:\ d +:\ d +.\ d +].* Exception.*
      EndPattern: .*\.\.\. \d+ more
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Keys:
      - msg
      - time
    Regex: (\[(\S+)].*)
flushers:
  - Type: flusher_stdout
    OnlyStdout: true
    Tags: true
```

* Output

```Json
{
    "__tag__:__path__": "/home/test-log/regMulti.log",
    "time": "2022-07-07T10:43:27.360266763",
    "msg": "[2022-07-07T10:43:27.360266763] [ERROR] java.lang.Exception: exception happened\n[2022-07-07T10:43:27.360266763]    at com.aliyun.sls.devops.logGenerator.type.RegexMultiLog.f2(RegexMultiLog.java:108)\n[2022-07-07T10:43:27.360266763]    at java.base/java.lang.Thread.run(Thread.java:833)\n[2022-07-07T10:43:27.360266763]    ... 23 more"
}
{
    "__tag__:__path__": "/home/test-log/regMulti.log",
    "time": "2022-07-07T10:43:27.360266763",
    "msg": "[2022-07-07T10:43:27.360266763] Some user custom log"
}
{
    "__tag__:__path__": "/home/test-log/regMulti.log",
    "time": "2022-07-07T10:43:27.360266763",
    "msg": "[2022-07-07T10:43:27.360266763] Some user custom log"
}
```
