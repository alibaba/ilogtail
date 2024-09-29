FROM sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-build-linux:2.0.3 as build

USER root
WORKDIR /backend
COPY . .

RUN go env -w GOPROXY="https://goproxy.cn,direct" \
    && go mod tidy \
    && go build -o ConfigServer ./cmd


FROM centos:centos7.9.2009
RUN curl -L -o /etc/yum.repos.d/CentOS-Base.repo http://mirrors.aliyun.com/repo/Centos-7.repo

RUN yum update -y && yum upgrade -y && yum -y clean all && rm -fr /var/cache && rm -rf /core.*

WORKDIR /backend
COPY --from=build /backend/cmd /backend/cmd
COPY --from=build /backend/ConfigServer /backend/ConfigServer
ENV GIN_MODE=release

CMD sh -c "./ConfigServer"



