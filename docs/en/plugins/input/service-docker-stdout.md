# Container standard output

## Introduction

The 'service_docker_stdout' 'input' plug-in can collect logs from container standard output/Standard error streams, and the collected logs are saved in the 'content' field. Supports filtering containers to be collected by using container Meta information, and supports data processing operations such as multiline text segmentation and adding container Meta information,The overview is as follows:

Container Filtering. The relationship between each filter condition is AND.

* You can specify the container to be collected by using the container Label whitelist. The relationship between multiple whitelists is OR.
* You can use the container Label blacklist to exclude containers that are not collected. The relationship between multiple blacklists is OR.
* You can specify the containers to be collected by using the environment variable whitelist. The relationship between multiple whitelists is OR.
* You can use the environment variable blacklist to exclude containers that are not collected. The relationship between multiple blacklists is OR.
* You can specify the container to be collected by Kubernetes Namespace name.
* You can specify the container to be collected by Kubernetes Pod name.
* You can specify the container to be collected by Kubernetes the container name.
* You can specify the container to be collected by using the Kubernetes Label whitelist. The relationship between multiple whitelists is OR.
* You can use the Kubernetes Label blacklist to exclude containers that you do not want to collect. The relationship between multiple blacklists is OR.

Data processing

* Supports collecting multiple rows of logs, such as Java Stack logs.
* Automatically associates Kubernetes Label information when reporting.
* Supports automatically associating container Meta information (such as container name, IP address, image, Pod, Namespace, and environment variables) when reporting.
* Supports automatically associating host Meta information (such as host name, IP address, and environment variables) when reporting.

## Version

[Stable](../stability-level.md)

## Configure parameters

### Basic parameters

| Parameter | Type | Required | Description |
| ----------------- | ------- | ---- | -------------------------------------------------------------------------- |
| Type | String | Yes | Plug-in Type, fixed to 'service_docker_stdout' |
| Stdout | Boolean | No | Whether to collect standard output stdout. The default value is 'true '. </p> |
| Stderr | Boolean | No | Whether to collect standard error information stderr. The default value is 'true '. </p> |
| StartLogMaxOffset | Integer | No | The length of historical data during the first collection. Unit: bytes. The recommended value is between [131072,1048576]. The default value is 128 × 1024 bytes. |

### Filter container parameters

| Parameter | Type | Required | Description |
| ----------------- | ---------------------------------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| IncludeLabel | Map, where LabelKey and LabelValue are of the String type | No | <p> Container Label whitelist, which is used to specify the container to be collected. The default value is null, indicating that the standard output of all containers is collected.If you want to set the container Label whitelist, LabelKey required, LabelValue optional. </p><ul><li> If the LabelValue is empty, all containers that contain LabelKey in the container Label match. </li><li><p> If the LabelValue is not empty,Only containers with LabelKey = LabelValue in the container Label match. </p><p> The default LabelValue is string matching, that is, only the values of the LabelValue and container Label are exactly the same. If the value starts with '^' and ends with '$,Regular match. For example, set the LabelKey to 'io.kubernetes.container.name' and set the LabelValue to '^(nginx\| cube)$' to match containers named nginx and cube.</p></li></ul><p>多个白名单之间为或关系，即只要容器Label满足任一白名单即可匹配。</p>                                                                |
| ExcludeLabel | Map, where LabelKey and LabelValue are of the String type | No | <p> Container Label blacklist, which is used to exclude containers that are not collected. The default value is null, indicating that no containers are excluded.If you want to set the container Label blacklist, LabelKey required, LabelValue optional. </p><ul><li> If the LabelValue is empty, containers that contain LabelKey in the container Label will be excluded. </li><li><p> If the LabelValue is not empty,Containers whose Label contains LabelKey = LabelValue are excluded. </p><p> The default LabelValue is string matching, that is, only the values of the LabelValue and container Label are exactly the same. If the value starts with '^' and ends with '$,Regular match. For example, set the LabelKey to 'io.kubernetes.container.name' and set the LabelValue to '^(nginx\| cube)$' to match containers named nginx and cube.</p></li></ul><p>多个黑名单之间为或关系，即只要容器Label满足任一黑名单对即可被排除。</p>                                                              |
| IncludeEnv | Map, where EnvKey and EnvValue are of String type | No | <p> Set the environment variable whitelist to specify the container to be collected. The default value is null, indicating that the standard output of all containers is collected.If you want to set the environment variable whitelist, EnvKey is required, EnvValue optional. </p><ul><li> If the EnvValue is empty, all containers containing EnvKey in the container environment variable match. </li><li><p> If the EnvValue is not empty,Containers whose environment variables contain EnvKey = EnvValue are matched. </p><p> The default EnvValue is string matching, that is, only the values of EnvValue and environment variables are exactly the same. If the value starts with '^' and ends with '$,Regular matching is used. For example, if EnvKey is set to 'nginx_service_port' and EnvValue is set to '^ (80\|6379)$', containers with service ports 80 and 6379 can be matched. </p></li></ul><p> The relationship between multiple whitelists is or,That is, as long as the environment variables of the container meet any key-value pair, they can be matched. </p> |
| ExcludeEnv | Map, where EnvKey and EnvValue are of String type | No | <p> The Blacklist of environment variables is used to exclude containers that are not collected. The default value is null, indicating that no containers are excluded.If you want to set a blacklist of environment variables, EnvKey is required, EnvValue optional. </p><ul><li> If the EnvValue is empty, the logs of containers containing EnvKey in the container environment variable will be excluded. </li><li><p> If the EnvValue is not empty,Containers whose environment variables contain EnvKey = EnvValue are excluded. </p><p> The default EnvValue is string matching, that is, only the values of EnvValue and environment variables are exactly the same. If the value starts with '^' and ends with '$,Regular matching is used. For example, if EnvKey is set to 'nginx_service_port' and EnvValue is set to '^ (80\| 6379)$', containers with service ports 80 and 6379 can be matched. </p></li></ul><p> The relationship between multiple blacklists is or,That is, container environment variables can be excluded as long as they meet any key-value pair. </p> |
| IncludeK8sLabel | Map, where LabelKey and LabelValue are of the String type | No | <p> Specify the container to be collected through the Kubernetes Label (defined in template.metadata) whitelist.If you want to set a Kubernetes Label whitelist, LabelKey required, LabelValue optional. </p><ul><li> If the LabelValue is empty, the containers that contain Kubernetes Label in the LabelKey match.</li><li><p>如果LabelValue不为空，则Kubernetes Label中包含LabelKey=LabelValue的容器才匹配。</p><p>LabelValue默认为字符串匹配，即只有LabelValue和Kubernetes Label的值完全相同才会匹配。If the value starts with '^' and ends with '$', it is a regular match. For example, if the LabelKey is set to 'app' and the LabelValue is set to '^(test1\| test2)$', the container that contains app:test1 and app:test2 in the matching Kubernetes Label is displayed.</p></li></ul><p>多个白名单之间为或关系，即只要Kubernetes Label满足任一白名单即可匹配。</p>      |
| ExcludeK8sLabel | Map, where LabelKey and LabelValue are of String type | No | <p> Exclude containers that are not collected by Kubernetes Label (defined in template.metadata) blacklist.If you want to set Kubernetes Label blacklist, LabelKey required. LabelValue optional. </p><ul><li> If the LabelValue is empty, containers that contain Kubernetes Label in the LabelKey are excluded.</li><li><p>如果LabelValue不为空，则Kubernetes Label中包含LabelKey=LabelValue的容器才会被排除。</p><p>LabelValue默认为字符串匹配，That is, only the values of LabelValue and Kubernetes Label are exactly the same. If the value starts with '^' and ends with '$', it is a regular match. For example, set LabelKey to 'app', set LabelValue to '^(test1\| test2)$',The container that matches the Kubernetes Label contains app:test1 and app:test2. </p></li></ul><p> there is an OR relationship between multiple blacklists, that is, Kubernetes Label can be excluded as long as any blacklist pair is satisfied.</p> |
| K8sNamespaceRegex | String | No | Specify the container to be collected by Namespace name. Regular matching is supported. For example, set it to '^(default\| nginx)$',All containers that match the nginx namespace, default namespace. |
| K8sPodRegex | String | No | Specify the container to be collected by Pod name. Regular matching is supported. For example, set it to '^(nginx-log-demo.*)$', which indicates that all containers in the Pod starting with nginx-log-demo are matched. |
| K8sContainerRegex | String | No | Specify the container to be collected by the container name (Kubernetes the container name is defined in spec.containers). Regular match is supported. For example, if you set this parameter to '^(container-test)$', all containers named container-test are matched. |

### Data processing parameters

| Parameter | Type | Required | Description |
| -------------------- |------------------------------------| ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| BeginLineRegex | String | No | <p> The regular expression that matches the beginning of the line. </p><p> This parameter is empty, indicating single-line mode.</p><p>如果该表达式匹配某行的开头，则将该行作为一条新的日志，否则将此行拼接到上一条日志。</p>                                                                                                                                                                       |
| BeginLineCheckLength | Integer | No | <p> The length of the first line match. Unit: bytes. </p><p> The default value is 10 × 1024 bytes.</p><p>如果行首匹配的正则表达式在前N个字节即可体现，推荐设置此参数，提升行首匹配效率。</p>                                                                                                                                                                    |
| BeginLineTimeoutMs | Integer | No | <p> The timeout period for the first line matching. Unit: milliseconds. </p><p> The default value is 3000 milliseconds.</p><p>如果3000毫秒内没有出现新日志，则结束匹配，将最后一条日志上传到日志服务。</p>                                                                                                                                                                       |
| MaxLogSize | Integer | No | <p> Maximum log length <strong>,</strong> The default value is 0,Unit: bytes. </p><p> The default value is 512 × 1024 bytes. </p><p> If the log length exceeds this value, you will not continue to search for the beginning of the line and upload it directly. </p> |
| ExternalK8sLabelTag | Map, where LabelKey and LabelValue are of String type | No | <p> Set Kubernetes Label (defined in template.metadata) after the log Label, the iLogtail adds Kubernetes Label related fields to the log. </p><p> For example, set LabelKey to app,LabelValue to 'k8s_label_app ',If the Pod contains Label 'app = serviceA', this information iLogtail is added to the log, that is, the k8s_label_app: serviceA field is added. If the label named app is not included, the k8s_label_app: field is added:.</p> |
| ExternalEnvTag | Map, where EnvKey and EnvValue are of the String type | No | <p> After the container environment variable log label is set, the iLogtail adds the container environment variable related fields to the log.</p><p>例如设置EnvKey为`VERSION`，EnvValue为`env_version`，当容器中包含环境变量`VERSION=v1.0.0`时，会将该信息以tag形式添加到日志中，即添加字段env_version: v1.0.0; If the environment variable named VERSION is not included, add the empty field env_version:. </p> |

### Data processing environment variables

| Environment variable | Type | Required | Description |
| ---------------------- | ------ | ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| ALIYUN\_log \_env \_tags | String | No | <p> After the global environment variable log label is set, iLogtail adds the relevant fields of the container environment variable where the iLogtail is located to the log. Separate multiple environment variable names with '\|.</p><p>例如设置为node_name\|node_ip，当iLogtail容器中暴露相关环境变量时，会将该信息以tag形式添加到日志中，即添加字段node_ip:172.16.0.1和node_name:worknode。</p> |

## Default log fields

All logs reported using this plug-in contain the following fields. Currently, changes are not supported.

| Field name | Description |
| ------------------- | ------------------------------------------ |
| \_time \_| log collection time, for example, '2021-02-02 T02:18:41.979147844Z '. |
| \_source \_| log source type, stdout or stderr. |
| \_Image \_name \_| image name |
| \_container\_name \_| container name |
| \_pod\_name\_       | Pod名                                       |
| \_namespace \_| the namespace of the Pod. |
| \_Pod \_UID \_| unique identifier of the Pod |

## Sample

### Example 1: filter containers through the blacklist and whitelist of container environment variables <a href = "#h3-url-1" id = "h3-url-1"></a>

Collects the standard output of containers whose environment variables contain 'nginx_service_port = 80' and do not contain 'pod_namespace = kube-system.

1\. Obtain environment variables.

You can log on to the host where the container is located to view the environment variables of the container. For more information, see [obtain container environment variables](https://help.aliyun.com/document\_detail/354831.htm#section-0a0-n4l-zhi).

{% hint style="info" %}
'Command prompt:'

docker inspect

crictl inspect

ctr -n k8s.io containers info
{% endhint %}

```plain
            "Env": [
                ...
                "NGINX_SERVICE_PORT=80",
```

2\. Create iLogtail collection configuration.

An example of iLogtail collection configuration is as follows.

```yaml
    inputs:
      - Type: service_docker_stdout
        Stdout: true
        Stderr: false
        IncludeEnv:
          NGINX_SERVICE_PORT: "80"
        ExcludeEnv:
          POD_NAMESPACE: "kube-system"
```

### Example 2: filter containers by using the container Label blacklist and whitelist <a href = "#h3-url-2" id = "h3-url-2"></a>

The container Label contains the standard output of the 'io.kubernetes.container.name:nginx' container and does not contain the 'io.kubernetes.pod.namespace:kube-system 'container.

1\. Obtain the container Label.

You can log on to the host where the container is located to view the Label of the container. For more information, see [get container Label](https://help.aliyun.com/document\_detail/354831.htm#section-7rp-xn8-crg).

```plain
            "Labels": {
                ...
                "io.kubernetes.container.name": "nginx",
```

2\. Create Logtail collection configuration.

An example of Logtail collection configuration is as follows.

```yaml
    inputs:
      - Type: service_docker_stdout
        Stdout: true
        Stderr: false
        IncludeLabel:
          io.kubernetes.container.name: "nginx"
        IncludeLabel:
          io.kubernetes.pod.namespace": "kube-system"
```

### Example 3: filter containers by Kubernetes Namespace name, Pod name, and container name <a href = "#h3-url-3" id = "h3-url-3"></a>

Collects the standard output of nginx containers in pods that start with nginx-in the default namespace.

1\. Obtain information about the Kubernetes level.

{% hint style="info" %}
'Command prompt:'

kubectl describe
{% endhint %}

```plain
Name: nginx-76d49876c7-r892w
Namespace:    default
...
Containers:
Ngcannot:
    Container ID:...
```

2\. Create Logtail collection configuration. An example of Logtail collection configuration is as follows.

```yaml
    inputs:
      - Type: service_docker_stdout
        Stdout: true
        Stderr: false
        K8sNamespaceRegex: "^(default)$"
        K8sPodRegex: "^(nginx-.*)$"
        K8sContainerRegex: "^(nginx)$"
```

### Example 4: filter containers by Kubernetes Label <a href = "#h3-url-4" id = "h3-url-4"></a>

Collect Kubernetes Label the standard output of all containers with app = nginx and without env = test.

1\. Obtain information about the Kubernetes level.

```plain
Name: nginx-76d49876c7-r892w
Namespace:    default
...
Labels:       app=nginx
              ...
```

2\. Create Logtail collection configuration. An example of Logtail collection configuration is as follows.

```yaml
    inputs:
      - Type: service_docker_stdout
        Stdout: true
        Stderr: false
        IncludeK8sLabel:
          app: "nginx"
        ExcludeK8sLabel:
          env: "^(test.*)$"
```

### Example 5: iLogtail collection configuration for multi-line logs <a href = "#title-asn-vuf-17z" id = "title-asn-vuf-17z"></a>

Collects the output in the Java exception stack (multi-line Log) of the standard error flow.

1\. Obtain a log sample.

```plain
2021-02-03 14:18:41.969 ERROR [spring-cloud-monitor] [nio-8080-exec-4] c.g.s.web.controller.DemoController : java.lang.NullPointerException
at org.apache.catalina.core.ApplicationFilterChain.internalDoFilter(ApplicationFilterChain.java:193)
at org.apache.catalina.core.ApplicationFilterChain.doFilter(ApplicationFilterChain.java:166)
...
```

Compile a regular expression based on the beginning of a line with a distinction.

First line: `2021-02-03 ...`

Regular: `\d+-\d+-\d+.*`

2\. Create Logtail collection configuration. An example of Logtail collection configuration is as follows.

```yaml
    inputs:
      - Type: service_docker_stdout
        Stdout: false
        Stderr: true
        BeginLineCheckLength: 10
        BeginLineRegex: "\\d+-\\d+-\\d+.*"
```
