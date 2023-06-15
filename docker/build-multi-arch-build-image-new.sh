#--build-arg VERSION="$VERSION" \
#--build-arg HOST_OS="$HOST_OS" \

#docker buildx build --platform linux/amd64,linux/arm64 \
#        -t sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/centos7-cve-fix:1.0.0 \
#        -o type=registry \
#        --no-cache -f Dockerfile.centos7-cve-fix .

docker buildx build --platform linux/amd64,linux/arm64 \
        -t sls-opensource-registry.cn-shanghai.cr.aliyuncs.com/ilogtail-community-edition/ilogtail-toolchain-linux:1.2.0 \
        -o type=registry \
        --no-cache -f Dockerfile.ilogtail-toolchain-linux .
