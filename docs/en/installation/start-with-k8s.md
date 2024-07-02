# Using Kubernetes

## Prerequisites

- You have already deployed a [Kubernetes cluster](https://minikube.sigs.k8s.io/docs/start/) or are using [minikube](https://minikube.sigs.k8s.io/docs/start/).
- You have access to a [kubectl](https://kubernetes.io/docs/tasks/tools/install-kubectl-linux/) tool to interact with the cluster.
  
## Collecting Kubernetes Container Logs

1. Create a namespace for iLogtail deployment:

   Save the following content as ilogtail-ns.yaml:

   ```yaml
   apiVersion: v1
   kind: Namespace
   metadata:
     name: ilogtail
   ```

   Alternatively, you can download the example configuration directly from:

   ```bash
   wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/start_with_k8s/ilogtail-ns.yaml
   ```

   Apply the configuration:

   ```bash
   kubectl apply -f ilogtail-ns.yaml
   ```

2. Create a ConfigMap and Secret for iLogtail configuration:

   Save the following content as ilogtail-user-configmap.yaml. This ConfigMap will be mounted as a config directory in the iLogtail container, allowing multiple collection configurations:

   ```yaml
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
           Stdout: true  # only collect stdout
           IncludeK8sLabel:
             app: nginx  # choose containers with this label
       processors:
         - Type: processor_regex  # structure log
           SourceKey: content
           Regex: '([\d\.:]+) - (\S+) \[(\S+) \S+\] \"(\S+) (\S+) ([^\\"]+)\" (\d+) (\d+) \"([^\\"]*)\" \"([^\\"]*)\" \"([^\\"]*)\"'
           Keys:
             - remote_addr
             - remote_user
             - time_local
             - method
             - url
             - protocol
             - status
             - body_bytes_sent
             - http_referer
             - http_user_agent
             - http_x_forwarded_for
       flushers:
         - Type: flusher_stdout
           OnlyStdout: true
   ```

   Save the following content as ilogtail-secret.yaml. This Secret is optional and used when you want to write logs to SLS:

   ```yaml
   apiVersion: v1
   kind: Secret
   metadata:
     name: ilogtail-secret
     namespace: ilogtail
   type: Opaque
   data:
     access_key_id:  # base64 accesskey id if you want to flush to SLS
     access_key:     # base64 accesskey secret if you want to flush to SLS
   ```

   Download the example configurations directly:

   ```bash
   wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/start_with_k8s/ilogtail-user-configmap.yaml
   wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/start_with_k8s/ilogtail-secret.yaml
   ```

   Apply the configurations:

   ```bash
   kubectl apply -f ilogtail-user-configmap.yaml
   kubectl apply -f ilogtail-secret.yaml
   ```

3. Create an iLogtail DaemonSet:

   Save the following content as ilogtail-daemonset.yaml:

   ```yaml
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
           - operator: Exists  # deploy on all nodes
         containers:
           - name: logtail
             env:
               - name: ALIYUN_LOG_ENV_TAGS  # add log tags from env
                 value: _node_name_|_node_ip_
               - name: _node_name_
                 valueFrom:
                   fieldRef:
                     apiVersion: v1
                     fieldPath: spec.nodeName
               - name: _node_ip_
                 valueFrom:
                   fieldRef:
                     apiVersion: v1
                     fieldPath: status.hostIP
               - name: cpu_usage_limit  # iLogtail's self monitor cpu limit
                 value: "1"
               - name: mem_usage_limit  # iLogtail's self monitor mem limit
                 value: "512"
               - name: default_access_key_id  # accesskey id if you want to flush to SLS
                 valueFrom:
                   secretKeyRef:
                     name: ilogtail-secret
                     key: access_key_id
                     optional: true
               - name: default_access_key  # accesskey secret if you want to flush to SLS
                 valueFrom:
                   secretKeyRef:
                     name: ilogtail-secret
                     key: access_key
                     optional: true
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
               - mountPath: /var/run  # for container runtime socket
                 name: run
               - mountPath: /logtail_host  # for log access on the node
                 mountPropagation: HostToContainer
                 name: root
                 readOnly: true
               - mountPath: /usr/local/ilogtail/checkpoint  # for checkpoint between container restart
                 name: checkpoint
               - mountPath: /usr/local/ilogtail/config/local  # mount config dir
                 name: user-config
                 readOnly: true
             command:
               - /usr/local/ilogtail/ilogtail_control.sh
               - "start_and_block"
               - "-enable_containerd_upper_dir_detect=true"
               - "-dirfile_check_interval_ms=5000"
               - "-logtail_checkpoint_check_gc_interval_sec=120"
   ```

   Download the example configuration directly:

   ```bash
   wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/start_with_k8s/ilogtail-daemonset.yaml
   ```

   Apply the configuration:

   ```bash
   kubectl apply -f ilogtail-daemonset.yaml
   ```

4. Deploy a test Nginx deployment:

   Save the following content as nginx-deployment.yaml:

   ```yaml
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

   Download the example configuration directly:

   ```bash
   wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/start_with_k8s/nginx-deployment.yaml
   ```

   Apply the configuration:

   ```bash
   kubectl apply -f nginx-deployment.yaml
   ```

5. Send a request to generate sample logs:

   ```bash
   kubectl exec nginx-<pod-id> -- curl localhost/hello/ilogtail
   ```

6. View the collected logs from the test container:

   ```bash
   kubectl logs ilogtail-ds-<pod-id> -n ilogtail
   ```

   The output would be:

   ```json
   {
     "_time_": "2022-07-15T00:36:48.489153485+08:00",
     "_source_": "stdout",
     "_image_name_": "docker.io/library/nginx:latest",
     "_container_name_": "nginx",
     "_pod_name_": "nginx-76d49876c7-r892w",
     "_namespace_": "default",
     "_pod_uid_": "07f75a79-da69-40ac-ae2b-77a632929cc6",
     "_container_ip_": "10.223.0.154",
     "remote_addr": "::1",
     "remote_user": "-",
     "time_local": "14/Jul/2022:16:36:48",
     "method": "GET",
     "url": "/hello/ilogtail",
     "protocol": "HTTP/1.1",
     "status": "404",
     "body_bytes_sent": "153",
     "http_referer": "-",
     "http_user_agent": "curl/7.74.0",
     "http_x_forwarded_for": "-",
     "__time__": "1657816609"
   }
   ```

## Collecting Templates

- Learn about the structure of collection configurations: [Collection Configurations](../configuration/collection-config.md)
- Explore more Kubernetes log collection templates (collecting file logs from containers): <https://github.com/alibaba/ilogtail/blob/main/k8s_templates>
- Refer to more combined log collection and processing configuration examples: <https://github.com/alibaba/ilogtail/blob/main/example_config>
