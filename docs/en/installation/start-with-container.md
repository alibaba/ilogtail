# Using Docker

## Prerequisites

Before you proceed, make sure you have [installed Docker](https://docs.docker.com/engine/install/).

## Collecting Docker Container Logs

1. **Prepare the iLogtail Configuration Directory**

   Create a `config` directory and inside it, create `file_simple.yaml` and `stdout_simple.yaml` files.

   In `file_simple.yaml`, configure iLogtail to collect the `simple.log` file in the container and output to standard output.

   ```yaml
   enable: true
   inputs:
     - Type: input_file          # File input type
       FilePaths:
         - ./simple.log
   flushers:
     - Type: flusher_stdout    # Standard output stream output type
       OnlyStdout: true
   ```

   In `stdout_simple.yaml`, configure iLogtail to collect container standard output and write it to the `simple.stdout` file.

   ```yaml
   enable: true
   inputs:
     - Type: service_docker_stdout # Docker container standard output stream input type
       Stderr: false               # Don't collect standard error stream
       Stdout: true                # Collect standard output stream
   flushers:
     - Type: flusher_stdout        # Standard output stream output type
       FileName: simple.stdout     # Redirect file name
   ```

   You can also download the sample configurations directly from the following links:

   ```bash
   mkdir config && cd config
   wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/start_with_docker/config/file_simple.yaml
   wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/start_with_docker/config/stdout_simple.yaml
   cd -
   ```

2. **Start the iLogtail Container and Mount Configuration Directory**

   Run the iLogtail container in the background, mounting the configuration directory:

   ```bash
   docker run -d --name docker_ilogtail \
     -v /:/logtail_host:ro \
     -v /var/run:/var/run \
     -v /var/lib/docker_ilogtail/checkpoint:/usr/local/ilogtail/checkpoint \
     -v $(pwd)/config:/usr/local/ilogtail/config/local \
     sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:latest
   ```

   The `-d` flag runs the container in the background, and `--name` is used to specify a container name for reference.

   Mount points:
   - `/` is mounted to `/logtail_host` in the container for iLogtail to collect container logs.
   - `/var/run` is mounted to `/var/run` in the container for iLogtail to communicate with the container engine.
   - `/var/lib/docker_ilogtail/checkpoint` is mounted to persist the iLogtail state across container restarts.
   - The configuration directory is mounted to `/usr/local/ilogtail/config/local` in the container.

3. **View iLogtail Docker Container's Standard Output Logs**

   ```bash
   docker logs docker_ilogtail
   ```

   The output might look like:

   ```text
   delay stop seconds:  0
   ilogtail started. pid: 10
   register fun v2 0xa34f3c 0xa34f86 0xa34fdc 0xa35576
   2022-07-14 16:23:17 DEBUG Now using Go's stdlib log package (via loggers/mappers/stdlib).
   load log config /usr/local/ilogtail/plugin_logger.xml
   recover stderr
   recover stdout
   ```

4. **Enter the iLogtail Container**

   ```bash
   docker exec -it docker_ilogtail bash
   ```

5. **View Collected Standard Output**

   ```bash
   cat /usr/local/ilogtail/simple.stdout
   ```

   The output might show:

   ```text
   2022-07-14 16:23:20 {"content":"delay stop seconds:  0","_time_":"2022-07-14T16:23:17.704235928Z","_source_":"stdout","_image_name_":"sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.0","_container_name_":"docker_ilogtail","_container_ip_":"172.17.0.2","__time__":"1657815797"}
   2022-07-14 16:23:20 {"content":"ilogtail started. pid: 10","_time_":"2022-07-14T16:23:17.704404952Z","_source_":"stdout","_image_name_":"sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.0","_container_name_":"docker_ilogtail","_container_ip_":"172.17.0.2","__time__":"1657815797"}
   2022-07-14 16:23:20 {"content":"recover stdout","_time_":"2022-07-14T16:23:17.847939016Z","_source_":"stdout","_image_name_":"sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail:1.1.0","_container_name_":"docker_ilogtail","_container_ip_":"172.17.0.2","__time__":"1657815797"}
   ```

6. **Create a Sample Log**

   ```bash
   echo 'Hello, iLogtail!' >> /root/simple.log
   ```

7. **View Collected Container File Logs**

   Exit the container and run the following command on the host machine:

   ```bash
   docker logs docker_ilogtail
   ```

   The output will now include the additional log entry from the file:

   ```text
   2022-07-14 16:26:20 {"__tag__:__path__":"/root/simple.log","content":"Hello, iLogtail!","__time__":"1657815980"}
   ```

## Collection Templates

Learn about the collection configuration structure: [Collection Configuration](../configuration/collection-config.md)

Explore more combinable log collection and processing configuration examples: <https://github.com/alibaba/ilogtail/blob/main/example_config>
