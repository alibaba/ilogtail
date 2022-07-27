# ilogtail-controller

## 快速启动
```
go build
nohup ./ilogtail-controller &
```

## 命令

* `127.0.0.1:8899/machine/register/:uid` 

    GET请求，机器向controller发送注册/心跳，需要附带机器uid。
* `127.0.0.1:8899/machine/addConfig/`

    POST请求，向机器发送config。请求体为选择的config的name数组，和目标machine的uid数组。
* `127.0.0.1:8899/machine/delConfig/`
  
    POST请求，让机器删除config。请求体为选择的config的name数组，和目标machine的uid数组。
* `127.0.0.1:8899/machine/list/`
  
    GET请求，获取本地机器列表。
* `127.0.0.1:8899/local/addConfig/`

    POST请求，本地导入config。
* `127.0.0.1:8899/local/delConfig/`

    GET请求，删除本地config。