# Development environment

Although [source code compilation](docs/cn/installation/sources/build.md) has provided a convenient iLogtail compilation method, it is not suitable for development scenarios. Because continuous compilation and debugging are required during the development process, the speed of repeated full compilation is too slow,Therefore, you need to build a development environment that supports incremental compilation.

## Process Structure

To support plugin systems, iLogtail introduces two dynamic libraries, libPluginAdaptor and libPluginBase (hereinafter referred to as adaptor and base). The relationship between them and iLogtail is as follows:<br />
iLogtail dynamically depends on these two dynamic libraries (that is, they are not dependent in binary). During initialization, iLogtail attempts to dynamically load them using dynamic library interfaces (such as dlopen) to obtain the required symbols. <br/>
Adaptor acts as an intermediate layer, and both iLogtail and base depend on it. iLogtail register callbacks with adaptor, adpator record these callbacks and expose them to base as interfaces. <br/>
Base is the main body of the plugin system, which includes the necessary functions of the plugin system, such as collection, processing, aggregation, and output (which can be regarded as one of them if submitted to the iLogtail). <br/>
Therefore, the complete iLogtail contains three binary files: ilogtail, libPluginAdaptor.so, and libPluginBase.so.

![image.png](https://sls-opensource.oss-us-west-1.aliyuncs.com/ilogtail/ilogtail-adapter-cgo.png)

## Directory structure <a name = "iKc61"></a>

The general directory structure of the iLogtail is as follows:

```shell
.
├── core                  # C ++ core code
│   ├── CMakeLists.txt    # C ++ project description file
│   └── ilogtail.cpp      # C ++ main function
├── plugins               # Go plugin code
├── go.mod                # Go project description file
├── docker                # dorker image description directory for auxiliary compilation
├── scripts               # script directory for auxiliary compilation
└── Makefile              # compilation description file
```

The core directory contains iLogtail C ++ core code, and ilogtail.cpp is the main function entry file. CMake is used to describe the C ++ project. CMakeLists.txt is the general entry. CMakeLists.txt is also included in each subdirectory to describe the compilation target in the subdirectory.

The top-level directory itself is a Go Project. The project is iLogtail plugin, and go.mod is its description file. The plugin code body is in the plugins directory.

The docker directory and the scripts directory are the image description directory and the script Directory of the secondary compilation. Makefile is the compilation description file of the entire iLogtail and encapsulates the compilation Command.

## Develop an image

iLogtail relies on many third-party libraries (see [compilation dependencies](./dependencies.md)). In order to simplify the compilation process, ilogtail provides precompiled dependent images for auxiliary compilation. The URL for developing the image is in 'sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux`，可通过下面命令获取。

```shell
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux
```

gcc and go compilers are pre-installed in the development Image, git is updated, clang-format and go-tools are uniformly installed for code style, gdb, go-delve and other tools are also installed for easy debugging. For the convenience of domestic developers, GOPROXY = "[https://goproxy.cn,direct"](https://goproxy.cn,direct")。

```shell
$ gcc --version
gcc (GCC) 9.3.1 20200408 (Red Hat 9.3.1-2)
$ go version
go version go1.16.15 linux/amd64
$ git --version
git version 2.29.3
```

The compilation of the C ++ core depends on the/opt/logtail/deps/Directory. The path is DEFAULT_DEPS_ROOT of CMakeLists.txt (see./core/dependencies.cmake to find the variable to modify the path).

```shell
$ ls /opt/logtail/deps/
bin  include  lib  lib64  share  ssl
```

To install more tools or compile dependencies, you can superimpose images on the development image to create a custom development image.

```dockerfile
from sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux
yum -y install ...
go install ...
```

## Use VS Code to build a development environment

[VS Code](https://code.visualstudio.com/) use [Remote Development](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.vscode-remote-extensionpack) plugins can be developed remotely, in images, or even in remote + images. The Function developed in images enables the compilation environment to be unified among different deployments.Because VS Code is free and powerful, we choose VS Code to create a consistent and portable development environment for iLogtail.

### 1. Install the plugin <a name = "zpMpx"></a>

Search for "VS Code" in the Marketplace of the Remote Development to install the plugin. <br/>![image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/vscode-remote-development-plugin.png)

### 2. Create image development environment configuration <a name = "S3QyX"></a>

Create the. devcontainer' directory in the top-level Directory of the iLogtail code library, and create the 'devcontainer.json' file in it. The file content is as follows:

```json
{
  "image": "sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux:2.0.1",
  "customizations": {
    "vscode": {
      "extensions": [
        "golang.Go",
        "ms-vscode.cpptools-extension-pack",
        "DavidAnson.vscode-markdownlint",
        "redhat.vscode-yaml"
      ]
    }
  }
}
```

image specifies the development image address of the ilogtail, and customizations.vscode.extensions specifies the plugin of the development environment. Some plugins are described as follows. Developers can also modify them according to their own habits. [welcome to discuss](https:// github.com/alibaba/ilogtail/discussions/299)。

| **plugin name** | **purpose** |
| --- | --- |
| golang.Go | Go Development essential plugins |
| ms-vscode.cpptools-extension-pack | C++ Development essential plugins |
| DavidAnson.vscode-markdownlint | Markdown linting |
| redhat.vscode-yaml | YAML linting |

### 3. Open the code library in the container <a name = "Rsqu1"></a>

Use `Shift + Command + P`(Mac) or `Ctrl + Shift + P`(Win) to open the Command panel, enter `reopen`, choose `Remote-Containers: Reopen in Container`。<br />![image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/reopen-in-palette.png)<br /> or if the following prompt appears, you can directly click to reopen the container. <br/>![image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/reopen-in-tip.png)<br /> It is slow to open it for the first time because you need to download and compile the image and install the plugin,It will be very fast to open it again later. Follow the instructions to Build the image. <br /> after completing the above steps, we can use VS Code to edit the Code and compile the Code in it. <br /> Note: If you have pulled a compiled image before, you may need to trigger `remote-Containers: Rebuild Container Without Cache` to Rebuild.

### 4. Compile in the container <a name = "wEf4T"></a>

Open a New Terminal (if you cannot find it, you can Terminal in the command panel and select a new one)<br />![Image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/terminal.png)

- Compile the Go plugin

```bash
go mod tidy # to update the plugin library
make plugin_local   # start here after each plugin code update
```

![image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/go-compile-result.png)<br /> If you only modify the plugin code, you only need to execute the last line of commands to perform incremental compilation.

- Compile C ++ code

```bash
mkdir -p core/build  # if it has not been built before
cd core/build
cmake ..            # if you add or delete files, modify CMakeLists.txt and run it again.
make -sj$(nproc)    # start here after each core code update
```

![image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/c%2B%2B-compiling.png)<br />![image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/c%2B%2B-compile-result.png)

If you only modify the core code, you only need to execute the last line of commands to perform incremental compilation.

The default compilation option code may be optimized. If you need to Debug, we recommend that you modify the CMAKE_BUILD_TYPE switch. Replace the second behavior

```bash
cmake -D CMAKE_BUILD_TYPE=Debug ..
```

Similarly, if code optimization is required, change the Debug to Release.

The default compilation switch does not enable UT. To compile UT, you need to add the BUILD_LOGTAIL_UT switch. Replace the second behavior

```bash
cmake -DBUILD_LOGTAIL_UT=ON ..
```

- Create an image

Currently, docker is not installed. Compile it directly on the host.

### 5. Obtain the compilation output <a name = "X0fef"></a>

Because the VS Code directly mounts the Code library directory to the image, the host can directly access the compilation output in the image. <br/>![Image.png](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/images/developer-guide/development-environment/artifacts-on-host.png)<br />Now, images are run as root. Therefore, you may need to run 'sudo chown-R $USER .'on the host to fix the permissions.

You can copy the C ++ core build results to the './output' directory to assemble the complete build results.

```bash
cp -a ./core/build/ilogtail ./output
cp -a ./core/build/go_pipeline/libPluginAdapter.so ./output
```

The structure of the final assembled './output' directory is as follows:

```bash
./output
├── ilogtail (main program)
├── libPluginAdapter.so (plugin interface)
├── libPluginBase.h
└── libPluginBase.so (plugin lib)
```

## Compile directly using images <a name = "VsYKL"></a>

If it is not convenient to use the VS Code, you can also manually mount the Code library directory through Docker to start the compilation image and enter the compilation.

### 1. Enter the top-level Directory of the source code

### 2. Create a compilation container and mount the code directory

```bash
docker run --name ilogtail-build -d \
  -v `pwd`:/src -w /src \
  sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux:2.0.1 \
  bash -c "sleep infinity"
```

### 3. Enter the container

```bash
docker exec -it ilogtail-build bash
```

### 4. Compile in containers

The subsequent steps are the same as those of the VS Code.

## Use compilation output

On the host, the compiled output can be directly replaced with the corresponding file. For container scenarios, although you do not have the ability to create images in a compiled image, you can use the following method to quickly test without creating a new image.

### 1. Modify the official image entrypoint

Debugging based on the official image package. First, use bash to overwrite the entrypoint of the official image to prevent the container from exiting directly after killing the ilogtail.

- docker: specifies the CMD.

```bash
docker run -it --name docker_ilogtail -v /:/logtail_host:ro -v /var/run:/var/run aliyun/ilogtail:<VERSION> bash
```

- k8s：use command to recover entrypoint

```yaml
   command:
        - sleep
        - 'infinity'
# Delete the livenessProbe, or it will restart because the port cannot be detected.
```

### 2. Replace the binary files and so compiled by yourself with the container

Because ilogtail container is mounted with a host directory, you can replace the files in the container to access them.

```bash
# so scp compiled on the developer to the node where the container is located.
scp libPluginBase.so <user>@<node>:/home/<user>
```

The root path of the host is located in/logtail_host in the ilogtail container. Find the corresponding directory and copy it.

```bash
cp /logtail_host/home/<user>/libPluginBase.so /usr/local/ilogtail
```

## FAQ

### 1. Compilation error after code update

If a compilation error occurs, such

``` shell
/src/core/common/CompressTools.cpp:18:23: fatal error: zstd/zstd.h: No such file or directory
 #include <zstd/zstd.h>
                       ^
compilation terminated.
```

Make sure that the locally compiled image is the latest version. You can use the following command to update:

``` shell
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux
```

If you use VS Code for development, recompile and develop the container image. Use `Shift + Command + P`(Mac) or `Ctrl + Shift + P`(Win) to open the Command panel, enter rebuild, and select `Dev-Containers: Rebuild Without Cache and Reopen in Container`.
