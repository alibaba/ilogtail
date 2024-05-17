# Docker Images



## Preparation



- [Download the source code](download.md)
- [Install Docker](https://docs.docker.com/engine/install/)


## Image Creation



Image creation consists of two steps:

1. **Preparing the root directory for iLogtail-<VERSION>.tar.gz**
   - If using a released version, download the compressed package from the [iLogtail release notes](../release-notes.md).
   - To build with the latest local code, run `make dist`.

2. **Installing the iLogtail release tarball into the base image**
   - Execute the command `make docker`.

The default image name is `aliyun/ilogtail`. You can override it using the DOCKER_REPOSITORY environment variable, and the tag must match the iLogtail package version, which can be specified with the VERSION environment variable, for example:

```shell
$ VERSION=1.8.3 make dist
$ VERSION=1.8.3 make docker
$ docker image list
REPOSITORY        TAG     IMAGE ID       CREATED         SIZE
aliyun/ilogtail   1.8.3   c7977eb7dcc1   2 minutes ago   764MB
```
