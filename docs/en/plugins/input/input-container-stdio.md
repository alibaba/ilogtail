# Container Standard Output (Native Plugin)

## Introduction

The `input_container_stdio` `input` plugin allows for extracting logs from container standard output and standard error streams, with the content stored in the `content` field. It supports filtering containers based on container metadata and performs multi-line text splitting, adding container metadata, and is compatible with iLogtail versions 2.1 and later.

## Stability

[Stable](../stability-level.md)

## Configuration Parameters

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| Type | string | Yes | `/` | Plugin type. Fixed as `input_container_log`. |
| IgnoringStdout | Boolean | No | `false` | Whether to ignore standard output (stdout). |
| IgnoringStderr | Boolean | No | `false` | Whether to ignore standard error (stderr). |
| TailSizeKB | uint | No | `1024` | The size in kilobytes at which to start capturing logs from the standard output file. If the file size is smaller, it will start from the beginning, with a range of 0 to 10,485,760KB. |
| Multiline | object | No | `{}` | Multi-line aggregation options, see Table 1. |
| ContainerFilters | object | No | `{}` | Container filtering options. Multiple options are ANDed. See Table 2. |
| ExternalK8sLabelTag | map | No | `{}` | For containers deployed in a K8s environment, adds tags related to Pod labels to logs. Key is the Pod label name, and value is the tag name. E.g., add `app: k8s_label_app` for a tag like `app: serviceA` if the pod has the label `app=serviceA`. |
| ExternalEnvTag | map | No | `{}` | For containers deployed in a K8s environment, adds tags related to container environment variables to logs. Key is the environment variable name, and value is the tag name. E.g., add `VERSION: env_version` for a tag like `env_version: v1.0.0` if the container has the environment variable `VERSION=v1.0.0`. |
| flushTimeoutSecs | uint | No | `5` | When a file exceeds this time without a new complete log, the current read buffer is output as a log. |
| AllowingIncludedByMultiConfigs | bool | No | `false` | Whether to allow the current configuration to collect logs from containers that have already been matched by other configurations. |

* Table 1: Multi-line aggregation options

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| Mode | string | No | `custom` | Multi-line aggregation mode. Only `custom` is supported. |
| StartPattern | string | Yes (when `Multiline.Mode` is `custom`) | `empty` | Regular expression for the start of a line. |
| ContinuePattern | string | | `empty` | Regular expression for continuing a line. |
| EndPattern | string | | `empty` | Regular expression for the end of a line. |
| UnmatchedContentTreatment | string | No | `single_line` | How to handle unmatched log segments. Options: `discard` (discard), `single_line` (each unmatched line as a separate event). |

* Table 2: Container filtering options

| **Parameter** | **Type** | **Required** | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| K8sNamespaceRegex | string | No | `empty` | For K8s-deployed containers, specifies the namespace condition for the containers to be collected. Supports regex matching. |
| K8sPodRegex | string | No | `empty` | For K8s-deployed containers, specifies the Pod name condition for the containers to be collected. Supports regex matching. |
| IncludeK8sLabel | map | No | `{}` | For K8s-deployed containers, specifies the Pod label conditions for the containers to be collected. Multiple conditions are ORed. If not provided, all containers are collected. Supports regex matching. Key is the Pod label name, and value is the label value. |
| ExcludeK8sLabel | map | No | `{}` | For K8s-deployed containers, specifies exclusion conditions for the labels. Multiple conditions are ORed. If not provided, all containers are collected. Supports regex matching. Key is the Pod label name, and value is the label value. |
| K8sContainerRegex | string | No | `empty` | For K8s-deployed containers, specifies the container name condition. If not provided, all containers are collected. Supports regex matching. |
| IncludeEnv | map | No | `{}` | For K8s-deployed containers, specifies the environment variable conditions for the containers to be collected. Multiple conditions are ORed. If not provided, all containers are collected. Supports regex matching. Key is the environment variable name, and value is the variable value. |
| ExcludeEnv | map | No | `{}` | For K8s-deployed containers, specifies exclusion conditions for environment variables. Multiple conditions are ORed. If not provided, all containers are collected. Supports regex matching. Key is the environment variable name, and value is the variable value. |
| IncludeContainerLabel | map | No | `{}` | For K8s-deployed containers, specifies the container label conditions. Multiple conditions are ORed. If not provided, all containers are collected. Supports regex matching. Key is the container label name, and value is the label value. |
| ExcludeContainerLabel | map | No | `{}` | For K8s-deployed containers, specifies exclusion conditions for container labels. Multiple conditions are ORed. If not provided, all containers are collected. Supports regex matching. Key is the container label name, and value is the label value. |

## Default Log Fields

All logs reported by this plugin will have the following additional fields. Currently, these fields cannot be changed.

| Field Name | Description |
| --- | --- |
| \_time\_ | Timestamp of log collection, e.g., `2021-02-02T02:18:41.979147844Z`. |
| \_source\_ | Source type, either `stdout` or `stderr`. |

## Default Tag Fields

All log tags reported by this plugin will have the following additional fields. Currently, these fields cannot be changed.

| Field Name | Description |
| --- | --- |
| \_image\_name\_ | Image name. |
| \_container\_name\_ | Container name. |
| \_pod\_name\_ | Pod name. |
| \_namespace\_ | Pod's namespace. |
| \_pod\_uid\_ | Pod's unique identifier. |

## Examples

### Example 1: Filtering Containers by Environment Variables Blacklist <a id="h3-url-1"></a>

Collect logs from containers with environment variables containing `NGINX_SERVICE_PORT=80` and not containing `POD_NAMESPACE=kube-system`.

1\. Retrieve environment variables.

**How to do it:**

Log in to the host where the container is running. For more information, see [Getting Container Environment Variables](https://help.aliyun.com/document_detail/354831.htm#section-0a0-n4l-zhi).

2\. Create an iLogtail collection configuration.

**Example configuration:**

```yaml
inputs:
  - Type: input_container_stdio
    IgnoringStdout: false
    IgnoringStderr: true
    ContainerFilters:
      IncludeEnv:
        NGINX_SERVICE_PORT: "80"
      ExcludeEnv:
        POD_NAMESPACE: "kube-system"
```

### Example 2: Filtering Containers by Container Label Blacklist <a id="h3-url-2"></a>

Collect logs from containers with labels containing `io.kubernetes.container.name:nginx` and not containing `io.kubernetes.pod.namespace:kube-system`.

1\. Retrieve container labels.

**How to do it:**

* Log in to the host where the container is running. For more information, see [Getting Container Labels](https://help.aliyun.com/document_detail/354831.htm#section-7rp-xn8-crg).

2\. Create a Logtail collection configuration.

**Example configuration:**

```yaml
inputs:
  - Type: input_container_stdio
    IgnoringStdout: false
    IgnoringStderr: true
    ContainerFilters:
      IncludeContainerLabel:
        io.kubernetes.container.name: "nginx"
      IncludeContainerLabel:
        io.kubernetes.pod.namespace": "kube-system"
```

### Example 3: Filtering Containers by Kubernetes Namespace, Pod Name, and Container Name <a id="h3-url-3"></a>

Collect logs from the nginx containers in the default namespace whose Pod names start with `nginx-`.

1\. Retrieve Kubernetes-level information.

**How to do it:**

* Use `kubectl describe` for this.

2\. Create a Logtail collection configuration.

**Example configuration:**

```yaml
inputs:
  - Type: input_container_stdio
    IgnoringStdout: false
    IgnoringStderr: true
    ContainerFilters:
      K8sNamespaceRegex: "^(default)$"
      K8sPodRegex: "^(nginx-.*)$"
      K8sContainerRegex: "^(nginx)$"
```

### Example 4: Filtering Containers by Kubernetes Label <a id="h3-url-4"></a>

Collect logs from containers with `app=nginx` labels and without `env=test` labels.

1\. Retrieve Kubernetes-level information.

**How to do it:**

```plain
Name:         nginx-76d49876c7-r892w
Namespace:    default
...
Labels:       app=nginx
              ...
```

2\. Create a Logtail collection configuration.

**Example configuration:**

```yaml
inputs:
  - Type: input_container_stdio
    IgnoringStdout: false
    IgnoringStderr: true
    ContainerFilters:
      IncludeK8sLabel:
        app: "nginx"
      ExcludeK8sLabel:
        env: "^(test.*)$"
```

### Example 5: Collecting Multi-line Logs with iLogtail <a id="title-asn-vuf-17z"></a>

Collect logs from standard error stream with Java exception stack traces (multi-line logs).

1\. Get a log sample.

```plain
2021-02-03 14:18:41.969 ERROR [spring-cloud-monitor] [nio-8080-exec-4] c.g.s.web.controller.DemoController : java.lang.NullPointerException
  at org.apache.catalina.core.ApplicationFilterChain.internalDoFilter(ApplicationFilterChain.java:193)
  at org.apache.catalina.core.ApplicationFilterChain.doFilter(ApplicationFilterChain.java:166)
...
```

2\. Write a regex for the start of each line with discernible patterns.

**Pattern example:** `\d+-\d+-\d+.*`

3\. Create an iLogtail collection configuration.

**Example configuration:**

```yaml
inputs:
  - Type: input_container_stdio
    IgnoringStdout: false
    IgnoringStderr: true
    Multiline:
      StartPattern: \d+-\d+-\d+.*
```
