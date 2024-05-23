# GO Profile

## Introduction

The `service_go_profile` input plugin can collect performance data from Golang applications using the pprof profiling tool.

## Configuration Parameters

### Global Parameters

| Parameter          | Type, Default Value | Description                                                                                     |
|-----------------|------------------|--------------------------------------------------------------------------------------------------|
| Mode             | string, required  | Work mode, currently supports `host` and `kubernetes` for two different environments.            |
| Config           | object, required  | See sub-configurations for different work modes.                                                      |
| Interval         | int, 10           | Interval between performance data collection, in seconds.                                          |
| Timeout          | int, 15           | Timeout for performance data collection, in seconds.                                                 |
| BodyLimitSize    | int, 10240        | Maximum size limit for performance data body, in KB.                                             |
| EnabledProfiles  | []string, "cpu", "mem", "goroutines", "mutex", "block" | Types of performance data to collect.                                                            |
| Labels           | map[string]string, `{}` | Global labels to add to performance data.                                                         |

### Sub-Configuration: Host Mode

*For static host mode, specify service labels to indicate the service.*

- Sub-configuration

| Parameter        | Type, Default Value | Description                                                                                   |
|-----------|--------------------|-----------------------------------------------------------------------------------------------|
| Addresses | []Address, required | Array of Address objects.                                                                       |

- Address Configuration

| Parameter             | Type, Default Value                 | Description                                                                                   |
|------------------|----------------------------------|-----------------------------------------------------------------------------------------------|
| Host           | string, required                    | Instance address.                                                                              |
| Port           | int, required                       | Instance port.                                                                                 |
| InstanceLabels | map[string]string, `{}` | Specific instance performance data labels.                                                       |

### Sub-Configuration: Kubernetes Mode

*Containers to be collected must first declare the ILOGTAIL_PROFILE_PORT={PORT} environment variable, and then use the following configuration for secondary filtering.*

| Parameter                    | Type, Default Value | Optional, Description                                                                                     |
|-----------------------|------------------------------------|--------------------------------------------------------------------------------------------------|
| IncludeContainerLabel | Map<String, String>, `{}` | Container label whitelist for specifying containers to collect. Defaults to all containers. If set, LabelKey is required, LabelValue is optional. |
| ExcludeContainerLabel | Map<String, String>, `{}` | Container label blacklist for excluding containers. Defaults to none. If set, LabelKey is required, LabelValue is optional. |
| IncludeEnv            | Map<String, String>, `{}` | Environment variable whitelist for specifying containers to collect. Defaults to all containers. If set, EnvKey is required, EnvValue is optional. |
| ExcludeEnv            | Map<String, String>, `{}` | Environment variable blacklist for excluding containers. Defaults to none. If set, EnvKey is required, EnvValue is optional. |
| IncludeK8sLabel       | Map<String, String>, `{}` | Kubernetes label whitelist for specifying containers to collect based on template.metadata. If set, LabelKey is required, LabelValue is optional. |
| ExcludeK8sLabel       | Map<String, String>, `{}` | Kubernetes label blacklist for excluding containers based on template.metadata. If set, LabelKey is required, LabelValue is optional. |
| K8sNamespaceRegex     | String, `""`                        | Regular expression for Kubernetes namespace to collect containers from.                               |
| K8sPodRegex           | String, `""`                        | Regular expression for Pod name to collect containers from.                                          |
| K8sContainerRegex     | String, `""`                        | Regular expression for container name to collect (Kubernetes container name from spec.containers). |
| ExternalK8sLabelTag   | Map<String, String>, `{}` | Kubernetes label tags to add to performance data. If set, LabelKey is required, LabelValue is optional. |
| ExternalEnvTag        | Map<String, String>, `{}` | Environment variable tags to add to performance data. If set, EnvKey is required, EnvValue is optional. |

## Example

### Collecting Performance Data in Host Mode

- Configuration Example

```yaml
    inputs:
      - Type: service_go_profile
        Mode: host
        Labels:
          global: outer
          service: service
        Config:
          Addresses:
            - Host: "127.0.0.1"
              Port: 18689
              InstanceLabels:
                service: inner
```

- Collection Output

```plaintext
2023-04-03 14:04:52 [INF] [flusher_stdout.go:120] [Flush] [1.0#PluginProject_0##Config0,PluginLogstore_0]       {"name":"github.com/denisenkom/go-mssqldb/internal/cp.init /Users/evan/go/pkg/mod/github.com/denisenkom/go-mssqldb@v0.12.2/internal/cp/cp950.go","stack":"runtime.doInit /usr/local/go/src/runtime/proc.go\nruntime.doInit /usr/local/go/src/runtime/proc.go\nruntime.doInit /usr/local/go/src/runtime/proc.go\nruntime.doInit /usr/local/go/src/runtime/proc.go\nruntime.doInit /usr/local/go/src/runtime/proc.go\nruntime.main /usr/local/go/src/runtime/proc.go","stackID":"c4bbfc9dff64151d","language":"go","type":"profile_mem","dataType":"CallStack","durationNs":"0","profileID":"f6a443c0-cca0-45ac-b50a-b1e25a48fb74","labels":"{\"__name__\":\"service\",\"global\":\"outer\",\"instance\":\"inner\",\"job\":\"1_0_pluginproject_0__config0\"}","units":"bytes","valueTypes":"inuse_space","aggTypes":"sum","val":"1615794.00","__time__":"1680501892"}:  
```

### Collecting Performance Data in Kubernetes Mode

- Configuration Example

```yaml
    inputs:
      - Type: service_go_profile
        Mode: kubernetes
        Config:
          IncludeK8sLabel:
            app: golang-pull
        Labels:
          global: outer

```

- Collection Output

```text
2023-04-03 06:07:13 [INF] [flusher_stdout.go:120] [Flush] [1.0#PluginProject_0##Config0,PluginLogstore_0]       {"name":"runtime.memclrNoHeapPointers /usr/local/go/src/runtime/memclr_amd64.s","stack":"runtime.mallocgc /usr/local/go/src/runtime/malloc.go\nruntime.makeslice /usr/local/go/src/runtime/slice.go\nmain.memNormal /output/main.go\nmain.main.func1 /output/main.go","stackID":"36108813ff189cc4","language":"go","type":"profile_cpu","dataType":"CallStack","durationNs":"10000045532","profileID":"7bb6291c-b358-43fa-888a-dec7676552e4","labels":"{\"__name__\":\"golang-pull\",\"container\":\"golang-pull\",\"instance\":\"10.175.2.230:8080\",\"job\":\"1_0_pluginproject_0__config0\",\"label\":\"outer\",\"namespace\":\"profile\",\"pod\":\"golang-pull-6b946f8f6c-vhv9t\"}","units":"nanoseconds","valueTypes":"cpu","aggTypes":"sum","val":"20000000.00","__time__":"1680502023"}:
```
