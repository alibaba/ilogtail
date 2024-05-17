# Quick Start

## Collecting Host Logs

1. Download the pre-compiled iLogtail package, extract it, and enter the directory, which will be referred to as the deployment directory.

    ```bash
    wget https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/latest/ilogtail-latest.linux-amd64.tar.gz
    tar -xzvf ilogtail-latest.linux-amd64.tar.gz
    cd ilogtail-<version>
    ```

2. Configure iLogtail

    The `ilogtail_config.json` file in the deployment directory is the system parameter configuration file for iLogtail, and `config/local` is the local collection configuration directory for iLogtail.

    Create a `file_simple.yaml` file in the collection configuration directory to configure the collection of the `simple.log` file in the current directory and output it to standard output:

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

    You can also download the example configuration directly from the following address:

    ```bash
    cd ./config/local
    wget https://raw.githubusercontent.com/alibaba/ilogtail/main/example_config/quick_start/config/file_simple.yaml
    cd -
    ```

3. Start iLogtail in the background

    ```bash
    nohup ./ilogtail > stdout.log 2> stderr.log &
    ```

    This command redirects standard output to stdout.log for observation.

4. Generate sample logs

    ```bash
    echo 'Hello, iLogtail!' >> simple.log
    ```

5. View the collected log file

    ```bash
    cat stdout.log
    ```

    The output should be:

    ```json
    2022-07-15 00:20:29 {"__tag__:__path__":"./simple.log","content":"Hello, iLogtail!","__time__":"1657815627"}
    ```

## More Collection Configurations

Learn about the [collection configuration structure](../configuration/collection-config.md).

Explore more [combinable log collection and processing configuration examples](https://github.com/alibaba/ilogtail/tree/main/example_config).
