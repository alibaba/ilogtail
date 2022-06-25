#!/bin/sh
# called by generate-release-markdown.sh

set -ue
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 1.1.0" >&2
    exit 1
fi

INITPWD=$PWD
ROOTDIR=$(cd $(dirname $0) && cd .. && pwd)
version=$1

# Build version
sed -i "s/VERSION ?= .*/VERSION ?= $version/g" Makefile
sed -i "s/set (LOGTAIL_VERSION \".*\")/set (LOGTAIL_VERSION \"$version\")/g" \
    core/CMakeLists.txt \
    testfile

# Dockerfile
sed -i "s/ARG VERSION=.*/ARG VERSION=$version/g" docker/Dockerfile*

# Scripts
sed -i "s/VERSION=\${1:-.*}/VERSION=\${1:-$version}/g" scripts/*.sh
sed -i "s/image: aliyun\\/ilogtail:.*/image: aliyun\\/ilogtail:$version/g" test/engine/boot/compose.go

# Docs
sed -i "s/aliyun\\/ilogtail:[^\` ]*/aliyun\\/ilogtail:$version/g" \
    docs/zh/setup/README.md \
    docs/en/setup/README.md \
    docs/zh/guides/How-to-do-manual-test.md \
    docs/en/guides/How-to-do-manual-test.md \
    docs/zh/guides/How-to-build-with-docker.md \
    docs/en/guides/How-to-build-with-docker.md
sed -i -r "s/VERSION(.*)\`.*\`/VERSION\\1\`$version\`/g" \
    docs/zh/guides/How-to-build-with-docker.md \
    docs/en/guides/How-to-build-with-docker.md

cd "$INITPWD"