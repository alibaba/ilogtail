# Docker使用

## 使用前提

已安装[docker](https://docs.docker.com/engine/install/)

## 采集Docker容器日志

1. 准备 LoongCollector 配置目录

    新建配置目录`config`目录，在目录中创建`file_simple.yaml`和`stdout_simple.yaml`。

    在`file_simple.yaml`中配置采集容器中的`simple.log`到标准输出。

    ```yaml
    enable: true
    inputs:
      - Type: input_file          # 文件输入类型
        FilePaths: 
          - ./simple.log
    flushers:
      - Type: flusher_stdout    # 标准输出流输出类型
        OnlyStdout: true
    ```

    在`stdout_simple.yaml`中配置采集容器标准输出并输出到`simple.stdout`文件。

    ```yaml
    enable: true
    inputs:
      - Type: service_docker_stdout # 容器标准输出流输入类型
        Stderr: false               # 不采集标准错误流
        Stdout: true                # 采集标准输出流
    flushers:
      - Type: flusher_stdout        # 标准输出流输出类型
        FileName: simple.stdout     # 重定向文件名
    ```

    您也可以直接从下面的地址下载示例配置。

    ```bash
    mkdir config && cd config
    wget https://raw.githubusercontent.com/alibaba/loongcollector/main/example_config/start_with_docker/config/file_simple.yaml
    wget https://raw.githubusercontent.com/alibaba/loongcollector/main/example_config/start_with_docker/config/stdout_simple.yaml
    cd -
    ```

2. 启动 LoongCollector 容器，并挂载 LoongCollector 配置目录

    ```bash
    docker run -d --name docker_loongcollector \
      -v /:/logtail_host:ro \
      -v /var/run:/var/run \
      -v /var/lib/docker_loongcollector/checkpoint:/usr/local/loongcollector/checkpoint \
      -v `pwd`/config:/usr/local/ilogtail/config/local \
      sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:latest
    ```

    第1行-d参数表示后台启动iLogtail容器，--name指定容器名称以便引用。\
    第2行将主机/目录挂载到iLogtail容器中，iLogtail依赖`logtail_host`路径采集容器日志。\
    第3行将主机/var/run目录挂载到iLogtail容器中，iLogtail依赖/var/run目录与容器引擎通信。\
    第4行将主机目录挂载到容器中iLogtail的checkpoint目录，使采集状态在容器重启时可恢复。\
    第5行将配置目录挂载到iLogtail容器中。

3. 查看 docker_loongcollector 容器自身标准输出日志

    ```bash
    docker logs docker_loongcollector
    ```

    结果为

    ```text
    delay stop seconds:  0
    ilogtail started. pid: 10
    register fun v2 0xa34f3c 0xa34f86 0xa34fdc 0xa35576
    2022/07/14 16:23:17 DEBUG Now using Go's stdlib log package (via loggers/mappers/stdlib).
    load log config /usr/local/ilogtail/plugin_logger.xml
    recover stderr
    recover stdout
    ```

4. 进入iLogtail容器

    ```bash
    docker exec -it docker_loongcollector bash
    ```

5. 查看采集到的标准输出

    ```bash
    cat /usr/local/ilogtail/simple.stdout
    ```

    结果为

    ```text
    2022-07-14 16:23:20 {"content":"delay stop seconds:  0","_time_":"2022-07-14T16:23:17.704235928Z","_source_":"stdout","_image_name_":"sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.0","_container_name_":"docker_loongcollector","_container_ip_":"172.17.0.2","__time__":"1657815797"}
    2022-07-14 16:23:20 {"content":"ilogtail started. pid: 10","_time_":"2022-07-14T16:23:17.704404952Z","_source_":"stdout","_image_name_":"sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.0","_container_name_":"docker_loongcollector","_container_ip_":"172.17.0.2","__time__":"1657815797"}
    2022-07-14 16:23:20 {"content":"recover stdout","_time_":"2022-07-14T16:23:17.847939016Z","_source_":"stdout","_image_name_":"sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.0","_container_name_":"docker_loongcollector","_container_ip_":"172.17.0.2","__time__":"1657815797"}
    ```

6. 构造示例日志

    ```bash
    echo 'Hello, iLogtail!' >> /root/simple.log
    ```

7. 查看采集到的容器文件日志

    跳出容器，在宿主机上执行

    ```bash
    docker logs docker_loongcollector
    ```

    结果相比第3步的结果，多了

    ```text
    2022-07-14 16:26:20 {"__tag__:__path__":"/root/simple.log","content":"Hello, iLogtail!","__time__":"1657815980"}
    ```

## 采集模版

了解采集配置结构：[采集配置](../configuration/collection-config.md)

参考更多可组合的日志采集和处理配置样例：<https://github.com/alibaba/loongcollector/blob/main/example_config>
