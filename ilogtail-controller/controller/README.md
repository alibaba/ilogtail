# ilogtail-controller

## 简介

## 快速启动
```
cd ./ilogtail/ilogtail-controller/controller
go build
nohup ./controller > controller.log  &
```

## 命令

* `127.0.0.1:8899/machine/register/:instance_id` 

    GET请求，机器向controller发送注册/心跳，需要附带机器instance_id。
* `127.0.0.1:8899/machine/addConfig/`

    POST请求，向机器发送config。请求体为选择的config的name数组，和目标machine的instance_id数组。
* `127.0.0.1:8899/machine/delConfig/`
  
    POST请求，让机器删除config。请求体为选择的config的name数组，和目标machine的instance_id数组。
* `127.0.0.1:8899/machine/list/`
  
    GET请求，获取本地机器列表。
* `127.0.0.1:8899/local/addConfig/`

    POST请求，本地导入config。
* `127.0.0.1:8899/local/delConfig/`

    GET请求，删除本地config。