# 部署说明

本项目实现了config-server的后端（config-server）与前端（config-server-ui），并基于docker与docker-compose的方式进行部署

## 快速开始

进入`deployment`目录，运行`deployment-compose.yml`，启动三个容器（`mysql`、`config-server`、`config-server-ui`）

```shell
docker compose -f docker-compose.yml up -d
```

启动成功后，通过`http://{your-ip}:8080`即可实现前端页面的访问

## Agent启动

由于最新版本的[ilogtail](https://github.com/alibaba/ilogtail/releases/tag/v2.0.7)（截止到2.0.7）尚未提供支持v2心跳的功能，为了便于使用（测试）`config-server-ui`，本项目提供了基于docker启动Agent的脚本，复制`Dockefile-Agent`与`ilogtail_config.template.json`到Agent的目录下，
修改`.env`中的`${AGENT}`为Agent所在目录路径（Agent安装与启动见[quick-start](https://github.com/alibaba/ilogtail/blob/main/docs/cn/installation/quick-start.md)），并在`docker-compose.yml`添加
```yml
  agent:
    build:
      context: ${AGENT}
      dockerfile: Dockerfile-Agent
    image: ilogtail:openSource
    environment:
      CONFIG_SERVER_ADDRESSES: '["config-server:9090"]'
      TZ: Asia/Shanghai
    deploy:
      replicas: 3
    networks:
      - server
```

再次运行下面的命令，这将会启动3个ilogtail Agent。

```shell
docker compose -f docker-compose.yml up -d
```