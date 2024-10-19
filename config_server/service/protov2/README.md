# Go Proto

此为使用protoc命令生成go代码的文档

## 预备条件

安装好GO，并在环境变量中配置了`$GOROOT/bin`与`$GOPATH/bin`

## 快速开始

安装protoc（CentOs）

```shell
yum install -y protobuf-compiler
```

输入以下命令，若出现版本信息即安装成功

```shell
protoc --version
# libprotoc 3.5.0
```

安装`protoc-gen-go`插件，默认路径为`$GOPATH/bin`

```shell
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
```

进入`protocol/v2`文件夹

```shell
protoc --go_out=. agent.proto
```

```shell
protoc --go_out=. user.proto
```

