# Compiling

## Linux Build

### Preparing for Compilation

- [Download the source code](download.md)

- [Install Docker](https://docs.docker.com/engine/install/)

#### Building Targets <a name="veSpV"></a>

The `Makefile` describes all the build targets for the project, with the main ones being:

| **Target** | **Description** |
| --- | --- |
| core | Compiles only the C++ core |
| plugin | Compiles only the Go plugin |
| all | Builds the full iLogtail |
| dist | Packages for distribution |
| docker | Creates an iLogtail Docker image |
| plugin_local | Locally compiles the Go plugin |

To compile the selected target, use the `make <target>` command, and specify the version number with the `VERSION` environment variable if needed, like:

```shell
VERSION=1.8.3 make dist
```

If compilation errors occur, such as:

``` shell
/src/core/common/CompressTools.cpp:18:23: fatal error: zstd/zstd.h: No such file or directory
 #include <zstd/zstd.h>
                       ^
compilation terminated.
```

Ensure that the local Docker image is up to date. You can update it with:

``` shell
docker pull sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux
```

#### Building with the Docker Image

To build the full iLogtail using the Docker image, the command is `make all`, since `all` is the default build target, you can also simply use `make`. This command first cleans the `output` directory, then calls the `./scripts/gen_build_scripts.sh` script to generate the build scripts and Docker image description, saving them in the `./gen` directory. The Docker image is then created, with the build process happening inside the image, and finally, the compiled results are copied back to the `output` directory.

To quickly build the iLogtail executable and plugin:

1. Navigate to the top-level source code directory.

2. Run the `make` command.

3. Check the `output` directory for results.

```text
./output
├── ilogtail (main program)
├── libPluginAdapter.so (plugin interface)
├── libPluginBase.h
└── libPluginBase.so (plugin lib)
```

For incremental development in a development environment, refer to the [Development Environment Guide](../../developer-guide/development-environment.md).

#### Locally Compiling the Go Plugin

The Go plugin can be compiled directly on the host machine. Prior to compiling, you need to install the Go 1.16+ language development environment, as per the [official documentation](https://golang.org/doc/install).

After installation, to facilitate further development, follow the [document](https://golang.org/doc/code#Organization) to set up your development directory and GOPATH environment variables correctly.

```bash
go mod tidy   # Update plugin dependencies if needed
make plugin_local # Start from here after updating the plugin code
```

If you haven't modified the plugin dependencies, only the last line command is needed.

### Windows Build

#### Preparing for Compilation

- Set up a Windows machine
- Install Visual Studio Community Edition, preferably the 2017 version
- Install golang
- Install MinGW (select the appropriate version for your Windows machine)
- Download the corresponding version of the C++ boost library and install it
  - [boost_1_68_0-msvc-14.1-64.exe](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/prebuilt-dependencies/boost_1_68_0-msvc-14.1-64.exe)
  - [boost_1_68_0-msvc-14.1-32.exe](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/prebuilt-dependencies/boost_1_68_0-msvc-14.1-32.exe)
- Download the corresponding version of the ilogtail-deps build dependencies, unzip them
  - [64-bit dependencies](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/prebuilt-dependencies/ilogtail-deps.windows-x64.zip)
  - [32-bit dependencies](https://ilogtail-community-edition.oss-cn-shanghai.aliyuncs.com/prebuilt-dependencies/ilogtail-deps.windows-386.zip)

#### Modifying the Build Scripts

Replace the values of the five environment variables in the `ilogtail/scripts/windows64_build.bat` (or `windows32_build.bat`) script:

- ILOGTAIL_PLUGIN_SRC_PATH: Replace with the actual path on the build machine.

```bat
set ILOGTAIL_PLUGIN_SRC_PATH=%P1Path%
REM Change to where boost_1_68_0 is located
set BOOST_ROOT=C:\workspace\boost_1_68_0
REM Change to where ilogtail-deps.windows-x64 is located, path separator must be /
set ILOGTAIL_DEPS_PATH=C:/workspace/ilogtail-deps.windows-x64
REM Change to where cmake is located
set CMAKE_BIN=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake
REM Change to where devenv is located
set DEVENV_BIN=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.com
REM Change to where mingw is located
set MINGW_PATH=C:\workspace\mingw64\bin
```

#### Running the Build Script

Change to the `scripts` directory and run the `windows64_build.bat` (or `windows32_build.bat`) script. The build will take approximately 6 minutes to complete. The output includes:

- ilogtail.exe (main program)
- PluginAdapter.dll (plugin interface)
- PluginBase.dll (plugin lib)
- PluginBase.h
