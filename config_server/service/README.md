# ConfigServer

## 简介

## 运行

进入 Config Server 的 service 文件下

``` bash
go build -o ConfigServer
nohup ./ConfigServer > stdout.log 2> stderr.log &
```

## 测试及查看覆盖率

```bash
go test -cover github.com/alibaba/ilogtail/config_server/service/test -coverpkg github.com/alibaba/ilogtail/config_server/service/... -coverprofile coverage.out -v
go tool cover -func=coverage.out -o coverage.txt
# Visualization results: go tool cover -html=coverage.out -o coverage.html
cat coverage.txt
```
