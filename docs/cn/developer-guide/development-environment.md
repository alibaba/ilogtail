# 开发环境

虽然[源代码编译](docs/cn/installation/sources/build.md)已经提供了方便的iLogtail编译方法，但却不适合开发场景。因为开发过程中需要不断进行编译调试，重复全量编译的速度太慢，因此需要构建支持增量编译开发环境。

## 目录结构 <a name="iKc61"></a>

iLogtail的大致目录结构如下：

```shell
.
├── core                  # C++核心代码
│   ├── CMakeLists.txt    # C++项目描述文件
│   └── ilogtail.cpp      # C++主函数
├── plugins               # Go插件代码
├── go.mod                # Go项目描述文件
├── docker                # 辅助编译的镜像描述目录
├── scripts               # 辅助编译的脚本目录
└── Makefile              # 编译描述文件
```

core目录包含了iLogtail C++核心代码，ilogtail.cpp是其主函数入口文件。C++项目使用CMake描述，CMakeLists.txt是总入口，各子目录中还有CMakeLists.txt描述子目录下的编译目标。

顶层目录.本身就是一个Go项目，该项目为iLogtail插件，go.mod为其描述文件。插件代码主体在plugins目录。

docker目录和scripts目录分别为辅助编译的镜像描述目录和脚本目录。Makefile为整个iLogtail的编译描述文件，对编译命令进行了封装。

## 开发镜像

iLogtail依赖了诸多第三方库（详见[编译依赖](./dependencies.md)），为了简化编译流程ilogtail提供了预编译依赖的镜像辅助编译。开发镜像的地址在`sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/ilogtail-build-linux-amd64`，可通过下面命令获取。

```shell
docker pull sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/ilogtail-build-linux-amd64
```

开发镜像预先安装了gcc和go编译器，更新了git，为了代码风格统一安装了clang-format、go-tools，为了便于调试也安装了gdb、go-delve等工具。为方便国内开发者，预先设置了GOPROXY="[https://goproxy.cn,direct"](https://goproxy.cn,direct")。

```shell
$ gcc --version
gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-44)
$ go version
go version go1.16.15 linux/amd64
$ git --version
git version 2.29.3
```

C++核心的编译依赖在/opt/logtail/deps/目录下，该路径是CMakeLists.txt的DEFAULT_DEPS_ROOT（可以查看./core/dependencies.cmake找到修改路径的变量）。

```shell
$ ls /opt/logtail/deps/
bin  include  lib  lib64  share  ssl
```

如果需要安装更多工具或编译依赖，可以在开发镜像上镜像叠加，制作自定义的开发镜像。

```dockerfile
from sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/ilogtail-build-linux-amd64
yum -y install ...
go install ...
```

## 使用VS Code构建开发环境

[VS Code](https://code.visualstudio.com/)通过[Remote Development](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.vscode-remote-extensionpack)插件可以实现远程开发、在镜像中开发，甚至远程+镜像中开发，在镜像中开发的功能使得编译环境在不同部署间都能保持统一。由于VS Code免费而功能强大，因此我们选用VS Code来为iLogtail创建一致的、可移植的开发环境。

### 1. 安装插件  <a name="zpMpx"></a>

在VS Code的Marketplace中搜索“Remote Development”安装插件。<br />![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659940035893-bcb86ace-c6b2-453a-909f-9b906d1cfd2a.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=294&id=u32714f5f&margin=%5Bobject%20Object%5D&name=image.png&originHeight=646&originWidth=1166&originalType=binary&ratio=1&rotation=0&showTitle=false&size=123767&status=done&style=none&taskId=ud54f4874-76fb-4eca-babb-87d152e8edc&title=&width=529.9999885125596)

### 2. 创建镜像开发环境配置  <a name="S3QyX"></a>

在iLogtail代码库的顶层目录创建`.devcontainer`目录，并在里面创建`devcontainer.json`文件，文件的内容如下：

```json
{
  "image": "sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/ilogtail-build-linux-amd64:latest",
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

其中，image指定了ilogtail的开发镜像地址，customizations.vscode.extensions指定了开发环境的插件。部分插件介绍如下，开发者也可以按照自己的习惯进行修改。

| **插件名** | **用途** |
| --- | --- |
| golang.Go | Go开发必备插件 |
| ms-vscode.cpptools-extension-pack | C++开发必备插件 |
| DavidAnson.vscode-markdownlint | Markdown代码风格检查 |
| redhat.vscode-yaml | YAML语言支持 |

### 3. 在容器中打开代码库  <a name="Rsqu1"></a>

使用Shift + Command + P（Mac）或Ctrl + Shift + P（Win）打开命令面板，输入`reopen`，选择`Remote-Containers: Reopen in Container`。<br />![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659944991094-2683623d-3165-4953-8fa6-82f2c56917fe.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=63&id=u8227ef4a&margin=%5Bobject%20Object%5D&name=image.png&originHeight=138&originWidth=1462&originalType=binary&ratio=1&rotation=0&showTitle=false&size=23045&status=done&style=none&taskId=u1700aba5-6086-4215-bc8a-3831e5f5433&title=&width=664.54544014182)<br />或者若出现如下图提示，则可以直接点击在容器中重新打开。<br />![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659944851808-4ca87b97-d43e-41e7-ae30-7a1dfd6c279e.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=116&id=u675ad794&margin=%5Bobject%20Object%5D&name=image.png&originHeight=256&originWidth=1078&originalType=binary&ratio=1&rotation=0&showTitle=false&size=46758&status=done&style=none&taskId=u6671f647-4606-4a48-b2c4-24017be97d9&title=&width=489.99998937953626)<br />首次打开时会比较慢，因为要下载编译镜像并安装插件，后面再次打开时速度会很快。按照提示进行镜像Build。<br />完成上述步骤后，我们已经可以使用VS Code进行代码编辑，并在其中进行代码编译。<br />注：如果以前拉取过编译镜像，可能需要触发`Remote-Containers: Rebuild Container Without Cache`重新构建。

### 4. 在容器中进行编译  <a name="wEf4T"></a>

打开新Terminal（找不到的可以在命令面板中打Terminal，选择新开一个）<br />![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659947544982-af9008df-6bd4-4808-a42d-5cfd3f6346c0.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=45&id=uc3163db1&margin=%5Bobject%20Object%5D&name=image.png&originHeight=100&originWidth=2006&originalType=binary&ratio=1&rotation=0&showTitle=false&size=17267&status=done&style=none&taskId=ub288fe27-af01-40e2-8c6b-5197bc3e734&title=&width=911.8181620550554)

- 编译Go插件

```bash
make vendor       # 若需要更新插件库
make plugin_local # 每次更新插件代码后从这里开始
```

![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659947113467-9b3c3209-bec1-4bc6-ad4a-462dea33d63c.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=103&id=u8fdb824d&margin=%5Bobject%20Object%5D&name=image.png&originHeight=226&originWidth=1370&originalType=binary&ratio=1&rotation=0&showTitle=false&size=59631&status=done&style=none&taskId=u92534890-1de4-4cf1-b66f-d585a12f643&title=&width=622.7272592300229)<br />如果只是对插件代码进行了修改，则只需要执行最后一行命令即可增量编译。

- 编译C++代码

```bash
mkdir -p core/build # 若之前没有建过
cd core/build
cmake ..            # 若增删文件，修改CMakeLists.txt后需要重新执行
make -sj$(nproc)    # 每次更新core代码后从这里开始
```

![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659947164487-8266486b-1ed6-4224-896c-54839d20d2fb.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=84&id=u525bebe2&margin=%5Bobject%20Object%5D&name=image.png&originHeight=184&originWidth=1788&originalType=binary&ratio=1&rotation=0&showTitle=false&size=103041&status=done&style=none&taskId=ub8a6ef23-0526-4d99-8fa0-95f9ccb73b7&title=&width=812.7272551118839)![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659947380788-d9b7da88-1b01-4ddc-b56b-19a02251d106.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=108&id=uaa665a65&margin=%5Bobject%20Object%5D&name=image.png&originHeight=238&originWidth=2298&originalType=binary&ratio=1&rotation=0&showTitle=false&size=101069&status=done&style=none&taskId=u82aca763-5818-46a9-bb29-d3064979c9e&title=&width=1044.545431905542)<br />如果只是对core代码进行了修改，则只需要执行最后一行命令即可增量编译。<br />默认的编译开关没有打开UT，如果需要编译UT，可以打开开关。替换上述第2行为

```bash
cmake -DBUILD_LOGTAIL_UT=ON ..
```

- 制作镜像

目前不支持，没有安装docker，请直接在主机上进行编译。

### 5. 获取编译产出  <a name="X0fef"></a>

由于VS Code是直接将代码库目录挂载到镜像内的，因此主机上可以直接访问镜像内的编译产出。<br />![image.png](https://intranetproxy.alipay.com/skylark/lark/0/2022/png/31056853/1659947451606-ef5928fa-303f-40d3-bed2-5eb4f79292c3.png#clientId=ufd0bf718-58d2-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=139&id=uefcdb9e2&margin=%5Bobject%20Object%5D&name=image.png&originHeight=306&originWidth=2268&originalType=binary&ratio=1&rotation=0&showTitle=false&size=103664&status=done&style=none&taskId=u85b814c6-2d10-4fbe-b643-58ad2f20b0b&title=&width=1030.9090685647386)<br />目前，镜像使用的是root用户权限，因此在主机上可能需要执行`sudo chown -R $USER .`来修复一下权限。

可以将C++核心的构建结果拷贝到`./output`目录组装出完整的构建结果。

```bash
cp -a ./core/build/ilogtail ./output
cp -a ./core/build/plugin/libPluginAdapter.so ./output
```

最终组装的`./output`目录的结构如下

```text
./output
├── ilogtail (主程序）
├── libPluginAdapter.so（插件接口）
├── libPluginBase.h
└── libPluginBase.so (插件lib）
```

## 直接使用镜像编译  <a name="VsYKL"></a>

如果不方便使用VS Code，也可以通过Docker手动挂载代码库目录的方式，启动编译镜像进入编译。

### 1. 进入源代码顶层目录

### 2. 创建编译容器，并挂载代码目录

```bash
docker run --name ilogtail-build -d \
  -v `pwd`:/src -w /src \
  sls-registry.cn-beijing.cr.aliyuncs.com/sls-microservices/ilogtail-build-linux-amd64:latest \
  bash -c "sleep infinity"
```

### 3. 进入容器

```bash
docker exec -it ilogtail-build bash
```

### 4. 在容器内编译

后续步骤均与VS Code的操作步骤相同。
