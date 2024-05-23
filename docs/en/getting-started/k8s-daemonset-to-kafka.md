# Use DaemonSet to collect K8s container logs

'iLogtail' is a observable data collection 'Agent' developed by the Alibaba Cloud log service (SLS) team. It has many production-level features such as lightweight, high-performance, and automated configuration. It can be used to collect telemetry data in various environments such as physical machines, virtual machines, and 'Kubernetes.iLogtail has served the observability collection of tens of thousands of customer hosts and containers on Alibaba Cloud. Its core product lines, such as Taobao, Tmall, Alipay, Cainiao, and AMAP, are also default tools for collecting various observable data such as logs, monitoring, and Trace. Currently, iLogtail has tens of millions of installation capacity,Dozens of PB of observable data are collected every day, which are widely used in various scenarios such as online monitoring, problem analysis/positioning, Operation analysis, and security analysis, and have verified its strong performance and stability in actual combat.

In today's cloud-native era, we firmly believe that open source is the best development strategy for iLogtail and the way to release its maximum value. Therefore, we decided to open the 'iLogtail' source, hoping to work with many developers to build iLogtail into a world-class observable data collector.

## Log architecture of K8s <a href = "#x6yil" id = "x6yil"></a>

Logs are critical for building a data-driven application architecture. In Kubernetes distributed container environment, logs of various business containers are scattered everywhere. Users often want to have a centralized log management solution so that logs of different applications and formats can be processed and analyzed together.K8s has provided necessary basic resources and facilities for this purpose. This topic describes the log architecture of K8s and demonstrates how to collect logs by iLogtail.

![数据驱动的应用架构](<../.gitbook/assets/getting-started/k8s-daemonset-to-kafka/k8s-data-driven.png>)

K8s official recommend log architecture to output application logs to standard output stream (stdout) or standard error stream (stderr), and then Docker or Containerd + Kubelet redirects the log output to storage management. Kubernetes provides kubectl logs commands for users to query logs,This command can also accept the-p/-- previous parameter to query the log of the last container instance that has been exited. This parameter is especially useful when troubleshooting crashed or restarted containers.

However, if a pod is deleted from a node or the node crashes, the logs of all containers under it will be lost and users will no longer be able to query these logs. To avoid this, users should use a log collection and storage system independent of the lifecycle of business containers and nodes. Kubenertes there is no native solution,However, you can use the preferred log components to build log components by using Kubernetes API and controllers.

## Several methods for K8s to collect logs <a href = "#xaamx" id = "xaamx"></a>

Generally, logs are collected in the current K8s architecture in the following common ways:

1. Deploy a log collection Agent on each node
2. Deploy log collection containers in business pods in sidecar mode
3. Send logs directly to the server in the business application

Here we will only discuss the first method.

### Deploy log collection Agent on each node <a href = "#cgooc" id = "cgooc"></a>

In this way, the log collection Agent usually exists as a container that can access all logs on the node. Generally, the production cluster has many nodes, and each node needs to deploy a collection Agent. In this case, the simplest Deployment method is to directly use the Deployment provided by K8s for container orchestration.DaemonSet controller periodically checks the changes of nodes and automatically ensures that each node has only one collection Agent container.

![Image source:https://kubernetes.io/docs/concepts/cluster-administration/logging/](../.gitbook/assets/getting-started/k8s-daemonset-to-kafka/daemonset-logging-arch.png)

Using DaemonSet to collect K8s logs has the following advantages:

1. It occupies less node resources and does not increase as the number of business containers increases.
2. No intrusion to business applications. New applications do not need to be adapted.
3. Aggregate and send logs on a single node, which is more friendly to the receiver.

However, this method also has some limitations:

1. Cannot collect all types of PVC directories mounted in business containers, such as cloud disks
2. Cannot collect all types of Container Runtime, such as [Kata](https://katacontainers.io/)
3. Unable to support log traffic that exceeds the collection capability of a single Agent, such as 1 Gb/s.

In case of the above situation, other collection methods should be considered.

## Understand iLogtail principle of collecting container logs <a href = "#xnawr" id = "xnawr"></a>

iLogtail supports container data collection in all scenarios, including Docker and K8s environments. iLogtail use the [docker\_center plugin](https://github.com/alibaba/ilogtail/blob/main/helper/docker\_center.go) communicate with the container runtime on the node, discover the container list of the node, and maintain the mapping between the container and the log collection path. Then, for container standard output, iLogtail use the [input\_docker\_stdout plugin](https://github.com/alibaba/ilogtail/blob/main/plugins/input/docker/stdout/input\_docker\_stdout.go) collect logs, including container filtering and multiline splitting. For container files, use the [input\_docker\_event plugin](https://github.com/alibaba/ilogtail/blob/main/plugins/input/docker/event/input\_docker\_event.go) combined with C ++ kernel implementation, the former is responsible for container filtering, the latter provides efficient file discovery and collection capabilities. iLogtail supports multiple deployment methods such as DaemonSet, Sidecar, and CRD, providing flexible deployment capabilities for different scenarios. iLogtail uses the global container list and the Kubernetes CRI protocol to obtain container information,It is more lightweight in terms of permissions and component dependencies than other open sources and has higher collection efficiency.

![iLogtail采集K8s容器日志](<../.gitbook/assets/getting-started/k8s-daemonset-to-kafka/ilogtail-k8s-daemonset.png>)

iLogtail allows you to filter containers by using container tags, environment variables, K8s tags, Pod names, namespaces, and other methods. This allows you to collect container logs with great flexibility.

### Container filtering

Blacklist or whitelist

* Container Label
* K8s Label
* Environment variables

Regular match

* K8s Namespace name
* K8s Pod name
* Container name

### Data processing

* Supports collecting multiple rows of logs, such as Java Stack logs.
* Automatically associate Kubernetes Label information.
* Supports automatically associating container Meta information, such as container name, IP address, image, Pod, Namespace, and environment variables.
* Supports automatically associating host Meta information (such as host name, IP address, and environment variables).

## Deploy iLogtail to collect business logs to Kafka <a href = "#tsshm" id = "tsshm"></a>

This section will complete the first step of the data-driven application architecture, collecting and writing logs to Kafka. The configurations used in this topic can be found in [GitHub](https://github.com/alibaba/ilogtail/blob/main/k8s_templates/ilogtail-daemonset-kafka.yaml) download, container standard output plug-in detailed configuration can be moved to [iLogtail User Manual](https://ilogtail.gitbook.io/ilogtail-docs/plugins/input/input-docker-stdout).

### Prerequisites <a href = "#sra69" id = "sra69"></a>

1. Build a K8s cluster and have the [kubectl](https://kubernetes.io/docs/tasks/tools/install-kubectl-linux/) to access the K8s cluster
2. Build Kafka and have the to access [Kafka](https://kafka.apache.org/quickstart) consumer client
3. A [topic](https://ilogtail.gitbook.io/ilogtail-docs/getting-started/how-to-collect-to-kafka#hvouy) named access-log and error-log  has been created.

#### Step 1: Create a namespace and configuration file <a href = "#is9ro" id = "is9ro"></a>

Recommend deploy iLogtail in an independent named Space Station for management.

ilogtail-ns.yaml:

```yaml  {.line-numbers}
apiVersion: v1
kind: Namespace
metadata:
  name: ilogtail
```

```bash
kubectl apply -f ilogtail-ns.yaml
```

Currently, iLogtail community edition does not support configuration hot loading. Therefore, we first create a configuration and then start iLogtail container. If you need to modify the configmap, you can modify the ilogtail and restart the pod/container to make it take effect.

ilogtail-user-configmap.yaml:

```yaml  {.line-numbers}
apiVersion: v1
kind: ConfigMap
metadata:
  name: ilogtail-user-cm
  namespace: ilogtail
data:
  nginx_stdout.yaml: |
    enable: true
    inputs:
      - Type: service_docker_stdout
        Stderr: false
        Stdout: true
        IncludeK8sLabel:
          app: nginx
    flushers:
      - Type: flusher_kafka_v2
        Brokers:
          - <kafka_host>:<kafka_port>
        Topic: access-log
  nginx_stderr.yaml: |
    enable: true
    inputs:
      - Type: service_docker_stdout
        Stderr: true
        Stdout: false
        K8sNamespaceRegex: "^(default)$"
        K8sPodRegex: "^(nginx-.*)$"
        K8sContainerRegex: "nginx"
    flushers:
      - Type: flusher_kafka_v2
        Brokers:
          - <kafka_host>:<kafka_port>
        Topic: error-log
```

```bash
kubectl apply -f ilogtail-user-configmap.yaml
```

The ConfigMap here is expected to be mounted to the iLogtail container as a collection configuration directory in the form of a folder. Therefore, it can contain multiple iLogtail collection configuration files. From line 7 to line 19, it acts as a collection configuration. The standard output of nginx is collected to the Kafka access-log topic,10-33 is another collection configuration that outputs nginx standard errors to the Kafka error-log topic.

Lines 13-14 and 26-28 show how to filter containers for log collection. The former uses Kubernetes Label as the filter condition, while the latter uses Namespace, Pod, and Container names for filtering. For all supported configuration items, see [Container standard output](https:// iLogtail in the ilogtail user manual.gitbook.io/ilogtail-docs/plugins/input/input-docker-stdout)。

#### Step 2: deploy iLogtail DaemonSet <a href = "#mwxf7" id = "mwxf7"></a>

ilogtail-daemonset.yaml:

```yaml  {.line-numbers}
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: ilogtail-ds
  namespace: ilogtail
  labels:
    k8s-app: logtail-ds
spec:
  selector:
    matchLabels:
      k8s-app: logtail-ds
  template:
    metadata:
      labels:
        k8s-app: logtail-ds
    spec:
      tolerations:
      - key: node-role.kubernetes.io/master
        effect: NoSchedule
      containers:
      - name: logtail
        env:
          - name: cpu_usage_limit
            value: "1"
          - name: mem_usage_limit
            value: "512"
        image: >-
          sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:latest
        imagePullPolicy: IfNotPresent
        resources:
          limits:
            cpu: 1000m
            memory: 1Gi
          requests:
            cpu: 400m
            memory: 384Mi
        volumeMounts:
          - mountPath: /var/run
            name: run
          - mountPath: /logtail_host
            mountPropagation: HostToContainer
            name: root
            readOnly: true
          - mountPath: /usr/local/ilogtail/checkpoint
            name: checkpoint
          - mountPath: /usr/local/ilogtail/config/local
            name: user-config
            readOnly: true
      dnsPolicy: ClusterFirstWithHostNet
      hostNetwork: true
      volumes:
        - hostPath:
            path: /var/run
            type: Directory
          name: run
        - hostPath:
            path: /
            type: Directory
          name: root
        - hostPath:
            path: /lib/var/ilogtail-ilogtail-ds/checkpoint
            type: DirectoryOrCreate
          name: checkpoint
        - configMap:
            defaultMode: 420
            name: ilogtail-user-cm
          name: user-config
```

```bash
kubectl apply -f ilogtail-deployment.yaml
```

The 17-19 lines of the configuration file define the tolerance of the deployment node: it is not deployed on the master node.

23-26 the iLogtail is configured with the container environment variables. The cpu and memory limits are configured here. For complete system configuration instructions, please refer to [system parameters] (iLogtail.io/ilogtail-docs/configuration/system-config)。

Rows 31-36 define the range of resources allowed by the collection Agent container. If you need to collect a large number of log files, you need to relax the resource limits.

Some directories are mounted on lines 38-48 of the configuration file, as follows:

`/var/run`: the socket used by the iLogtail to communicate with the container runtime.

`/Logtail_host`: iLogtail obtain the logs of all containers on the node by mounting the host directory.

`/usr/local/ilogtail/checkpoint`: The status is persisted to the host disk iLogtail the container is restarted.

`/usr/local/ilogtail/config/local`: Mount the configuration in the configmap to the container

#### Step 3: deploy Nginx, send a test request and verify <a href = "#eiejy" id = "eiejy"></a>

nginx-deployment.yaml:

```yaml  {.line-numbers}
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
  namespace: default
  labels:
    app: nginx
spec:
  replicas: 1
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
        - image: 'nginx:latest'
          name: nginx
          ports:
            - containerPort: 80
              name: http
              protocol: TCP
          resources:
            requests:
              cpu: 100m
              memory: 100Mi
```

```bash
kubectl apply -f nginx-mock-deployment.yaml
```

Start the Kafka consumer to observe logs:

```bash
# In Terminal 1
bin/kafka-console-consumer.sh --topic access-log --bootstrap-server <kafka_host>:<kafka_port>
# In Terminal 2
bin/kafka-console-consumer.sh --topic error-log --bootstrap-server <kafka_host>:<kafka_port>
```

Send nginx several test requests, such:

```bash
kubectl exec nginx-76d49876c7-r892w -- curl localhost/hello/ilogtail
```

Check that the Kafka consumer has already output logs. As can be seen from the logs, the iLogtail marks the collected logs as necessary by default. For example, \_source\_marks whether the logs are standard output or standard error streams, and_container\_name_,_container\_name_, and \_container\_ip\_marks the containers where the logs are from.

```json
# In Terminal 1
{"Time":1657727155,"Contents":[{"Key":"content","Value":"::1 - - [13/Jul/2022:15:45:54 +0000] \"GET /hello/ilogtail HTTP/1.1\" 404 153 \"-\" \"curl/7.74.0\" \"-\""},{"Key":"_time_","Value":"2022-07-13T23:45:54.976593653+08:00"},{"Key":"_source_","Value":"stdout"},{"Key":"_image_name_","Value":"docker.io/library/nginx:latest"},{"Key":"_container_name_","Value":"nginx"},{"Key":"_pod_name_","Value":"nginx-76d49876c7-r892w"},{"Key":"_namespace_","Value":"default"},{"Key":"_pod_uid_","Value":"07f75a79-da69-40ac-ae2b-77a632929cc6"},{"Key":"_container_ip_","Value":"10.223.0.154"}]}
# In Terminal 2
{"Time":1657727190,"Contents":[{"Key":"content","Value":"2022/07/13 15:46:29 [error] 32#32: *6 open() \"/usr/share/nginx/html/hello/ilogtail\" failed (2: No such file or directory), client: ::1, server: localhost, request: \"GET /hello/ilogtail HTTP/1.1\", host: \"localhost\""},{"Key":"_time_","Value":"2022-07-13T23:45:54.976593653+08:00"},{"Key":"_source_","Value":"stderr"},{"Key":"_image_name_","Value":"docker.io/library/nginx:latest"},{"Key":"_container_name_","Value":"nginx"},{"Key":"_pod_name_","Value":"nginx-76d49876c7-r892w"},{"Key":"_namespace_","Value":"default"},{"Key":"_pod_uid_","Value":"07f75a79-da69-40ac-ae2b-77a632929cc6"},{"Key":"_container_ip_","Value":"10.223.0.154"}]}
```

#### Step 4: Configure regular parsing and structured logs <a href = "#s3zn3" id = "s3zn3"></a>

Unprocessed raw logs are inconvenient to use and have poor readability. You can use the built-in processing capability of the iLogtail to structure the logs.

替换ilogtail-user-configmap.yaml的1-19行，保存为ilogtail-user-configmap-processor.yaml。

```yaml  {.line-numbers}
  nginx_stdout.yaml: |
    enable: true
    inputs:
      - Type: service_docker_stdout
        Stderr: false
        Stdout: true
        IncludeK8sLabel:
          app: nginx
    processors:
      - Type: processor_regex
        SourceKey: content
        Regex: '([\d\.:]+) - (\S+) \[(\S+) \S+\] \"(\S+) (\S+) ([^\\"]+)\" (\d+) (\d+) \"([^\\"]*)\" \"([^\\"]*)\" \"([^\\"]*)\"'
        Keys:
          - remote_addr
          - remote_user
          - time_local
          - method
          - path
          - protocol
          - status
          - body_bytes_sent
          - http_referer
          - http_user_agent
          - http_x_forwarded_for
    flushers:
      - Type: flusher_kafka_v2
        Brokers:
          - <kafka_host>:<kafka_port>
        Topic: access-log
```

```bash
kubectl apply -f ilogtail-user-configmap-processor.yaml
```

Restart the iLogtail container to make it take effect.

```bash
kubectl exec -n ilogtail ilogtail-ds-krm8t -- /bin/sh -c "kill 1"
```

Send the test request again and observe the Kafka consumer access-log topic output. After a little formatting, you can see that each record is extracted into a structured log that is easy to read and use.

```json
{"Time":1657729579,"Contents":[
  {"Key":"_time_","Value":"2022-07-14T00:26:19.304905535+08:00"},
  {"Key":"_source_","Value":"stdout"},
  {"Key":"_image_name_","Value":"docker.io/library/nginx:latest"},
  {"Key":"_container_name_","Value":"nginx"},
  {"Key":"_pod_name_","Value":"nginx-76d49876c7-r892w"},
  {"Key":"_namespace_","Value":"default"},
  {"Key":"_pod_uid_","Value":"07f75a79-da69-40ac-ae2b-77a632929cc6"},
  {"Key":"_container_ip_","Value":"10.223.0.154"},
  {"Key":"remote_addr","Value":"::1"},
  {"Key":"remote_user","Value":"-"},
  {"Key":"time_local","Value":"13/Jul/2022:16:26:19"},
  {"Key":"method","Value":"GET"},
  {"Key":"url","Value":"/hello/ilogtail"},
  {"Key":"protocol","Value":"HTTP/1.1"},
  {"Key":"status","Value":"404"},
  {"Key":"body_bytes_sent","Value":"153"},
  {"Key":"http_referer","Value":"-"},
  {"Key":"http_user_agent","Value":"curl/7.74.0"},
  {"Key":"http_x_forwarded_for","Value":"-"}]}
```

### Collect files in the container

Some applications choose to print logs in containers and use the built-in log mechanism for rotation. iLogtail also supports log collection in this scenario. Here, we take collecting logs in json format as an example.

The prerequisites and deployment of the iLogtail DaemonSet are not described in detail, but only the configuration and verification process.

#### Step 1: Configure container log collection <a href = "#fcfc" id = "fcfc"></a>

ilogtail-user-configmap.yaml:

```yaml {.line-numbers}
apiVersion: v1
kind: ConfigMap
metadata:
  name: ilogtail-user-cm
  namespace: ilogtail
data:
  json_log.yaml: |
    enable: true
    inputs:
      - Type: input_file
        FilePaths: /root/log/json.log
        EnableContainerDiscovery: true
        ContainerFilters:
          IncludeContainerLabel:
            io.kubernetes.container.name: json-log
    processors:
      - Type: processor_json
        SourceKey: content
        KeepSource: false
        ExpandDepth: 1
        ExpandConnector: ""
    flushers:
      - Type: flusher_kafka_v2
        Brokers:
          - 39.99.61.125:9092
        Topic: json-log
```

Row 13 indicates that the collected files are from the container. Row 14-15 uses the container name to filter the target container. Lines 17-21 use the json processing plug-in to parse logs in a structured manner.

```bash
kubectl apply -f ilogtail-user-configmap.yaml
```

Restart the iLogtail container to make it take effect.

```bash
kubectl exec -n ilogtail ilogtail-ds-krm8t -- /bin/sh -c "kill 1"
```

#### Step 2: deploy the test container, generate logs, and verify <a href = "#rzyz" id = "rzyz"></a>

json-log-deployment.yaml:

json-log-deployment.yaml:

```yaml {.line-numbers}
apiVersion: apps/v1
kind: Deployment
metadata:
  labels:
    app: json-log
  name: json-log
  namespace: default
spec:
  replicas: 1
  selector:
    matchLabels:
      app: json-log
  template:
    metadata:
      labels:
        app: json-log
    spec:
      containers:
        - args:
            - >-
              mkdir -p /root/log; while true; do date +'{"time":"+%Y-%m-%d
              %H:%M:%S","message":"Hello, iLogtail!"}' >>/root/log/json.log;
              sleep 10; done
          command:
            - /bin/sh
            - '-c'
            - '--'
          image: 'alpine:3.9.6'
          name: json-log
          volumeMounts:
            - mountPath: /etc/localtime
              name: volume-localtime
      volumes:
        - hostPath:
            path: /etc/localtime
            type: ''
          name: volume-localtime
```

```bash
kubectl apply -f json-log-deployment.yaml
```

Start the Kafka consumer to observe logs:

```bash
bin/kafka-console-consumer.sh --topic json-log --bootstrap-server <kafka_host>:<kafka_port>
```

You can see that the consumer has Log output and structured parsing:

```json
{"Time":1658341942,"Contents":[
  {"Key":"__tag__:__path__","Value":"/root/log/json.log"},
  {"Key":"__tag__:__user_defined_id__","Value":"default"},
  {"Key":"__tag__:_container_ip_","Value":"10.223.0.189"},
  {"Key":"__tag__:_image_name_","Value":"docker.io/library/alpine:3.9.6"},{"Key":"__tag__:_container_name_","Value":"json-log"},
  {"Key":"__tag__:_pod_name_","Value":"json-log-5df95f9f84-dhj2l"},
  {"Key":"__tag__:_namespace_","Value":"default"},
  {"Key":"__tag__:_pod_uid_","Value":"e42818ef-75c4-4854-9fe0-4dd7c7f7ccd1"},
  {"Key":"time","Value":"+2022-07-21 02:32:22"},
  {"Key":"message","Value":"Hello, iLogtail!"}]}
```

## Summary <a href = "#qtdmu" id = "qtdmu"></a>

Above, we demonstrate how to use the basic capabilities provided by K8s to quickly build a cluster set of log collection infrastructure. We use the DaemonSet of K8s to automatically deploy iLogtail on each node, and use ConfigMap for configuration distribution. Powerful container filtering and metadata processing capabilities make iLogtail one of the best choices for collecting K8s container logs.In the future, we will further open-source iLogtail K8s Operator, manage configurations in the form of CRD, and further strengthen the control capability of iLogtail in the K8s environment.

A data-driven application architecture, from data collection to data application, data collection is only the beginning. If you have higher requirements for data transmission, storage, processing, and query, you can build a highly available and O & M-free data platform based on SLS.

## **About iLogtail**

* GitHub：[https://github.com/alibaba/ilogtail](https://github.com/alibaba/ilogtail)

* Community edition document: [https://ilogtail.gitbook.io/ilogtail-docs/about/readme](https://ilogtail.gitbook.io/ilogtail-docs/about/readme)

* Scan the Communication Group

![chatgroup](../.gitbook/assets/chatgroup.png)
