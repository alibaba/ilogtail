# Js Proto

此为通过`*.proto`文件生成js代码命令的文档

## 快速开始

安装protoc（CentOs）

```shell
yum install -y protobuf-compiler
```

输入命令，若出现版本信息即安装成功

```shell
protoc --version
# libprotoc 3.5.0
```

进入到`protocol/v2`文件夹

```
cd protocol/v2
```

运行下面两个命令

```shell
protoc --js_out=import_style=commonjs,binary:. agent.proto
```

```
protoc --js_out=import_style=commonjs,binary:. user.proto
```

