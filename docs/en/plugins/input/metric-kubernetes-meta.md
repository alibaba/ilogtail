# Kubernetes Meta

## Introduction

The `metric_meta_kubernetes` `input` plugin gathers metadata information from Kubernetes resources.

## Configuration Parameters

### Global Parameters

| Parameter                    | Type | Default | Description                                                                                     |
|-----------------------|------|--------|--------------------------------------------------------------------------------------------------|
| Pod                   | bool | true   | Collects metadata for Pods                                                                       |
| Node                  | bool | true   | Collects metadata for Nodes                                                                      |
| Service               | bool | true   | Collects metadata for Services                                                                     |
| Deployment            | bool | true   | Collects metadata for Deployments                                                                  |
| DaemonSet             | bool | true   | Collects metadata for DaemonSets                                                                   |
| StatefulSet           | bool | true   | Collects metadata for StatefulSets                                                                |
| Configmap             | bool | true   | Collects metadata for ConfigMaps                                                                   |
| Secret                | bool | true   | Collects metadata for Secrets                                                                      |
| Job                   | bool | true   | Collects metadata for Jobs                                                                         |
| CronJob               | bool | true   | Collects metadata for CronJobs                                                                     |
| Namespace             | bool | true   | Collects metadata for Namespaces                                                                   |
| PersistentVolume      | bool | true   | Collects metadata for PersistentVolumes                                                            |
| PersistentVolumeClaim | bool | true   | Collects metadata for PersistentVolumeClaims                                                       |
| StorageClass          | bool | true   | Collects metadata for StorageClasses                                                                |
| Ingress               | bool | true   | Collects metadata for Ingresses                                                                   |
| DisableReportParents  | bool | false  | Disables collection of parent information, e.g., if a Pod is from a Deployment, the parent is the Deployment |
| KubeConfigPath        | str  | ""     | Used for testing, configures the KubeConfig path                                                       |
| SelectedNamespaces    | list | []     | Collects resources within this namespace range, defaults to all namespaces                            |
| LabelSelectors        | str  | ""     | Supports label filtering based on k8s.io/apimachinery/pkg/labels/selector.go rules                     |
| IntervalMs            | int  | 300000 | Collection interval, default is 300 seconds                                                      |
| EnableOpenKruise      | bool | false  | Enables collection of enhanced Kubernetes resource metadata with OpenKruise                        |
| Labels                | map  | {}     | Allows adding custom labels to metadata                                                            |

## Example

### Collecting Kubernetes Metadata

#### Configuration Example

```yaml
inputs:
  - Type: metric_meta_kubernetes
    Labels:
      global: outer
```

#### Output Example

```text
2023-05-28 02:49:44 [INF] [flusher_stdout.go:120] [Flush] [##1.0##qs-demos$sls-mall__k8s-metas__sls-mall,sls-mall-metas] {"id":"27b7d494-2135-479e-be6a-6e192da849d3","type":"Pod","attributes":"{\"restart_count\":0,\"name\":\"golang-pull-6b946f8f6c-89kcl\",\"pod_ip\":\"\",\"container.0.image_name\":\"registry-vpc.cn-beijing.aliyuncs.com/log-service/logtail:profile-go-pull\",\"creation_time\":1681760673,\"phase\":\"Failed\",\"volume_claim\":\"\",\"resource_version\":\"377975647\",\"namespace\":\"profile\",\"workload\":\"golang-pull\",\"container.0.container_name\":\"golang-pull\"}","labels":"{\"app\":\"golang-pull\",\"pod-template-hash\":\"6b946f8f6c\",\"cluster\":\"sls-mall\"}","parents":"[\"Node:011574fd-110e-4705-bd3b-215e2bba6609:cn-beijing.192.168.32.120\",\"Deployment:3b9f3055-b12d-4c0b-b4ce-38dbb9ca832c:golang-pull\"]","__time__":"1685242184"}
```
