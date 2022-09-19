#!/usr/bin/env bash

# Copyright 2021 iLogtail Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -ue
set -o pipefail

# Currently, there are 4 supported docker categories, which are goc, build, development and production.
#
# goc: build goc server with Dockerfile_doc
# build: build core or plugin binary with Dockerfile_build
# development: build ilogtail development images.
# production: build ilogtail production images.
CATEGORY=$1
GENERATED_HOME=$2
VERSION=${3:-1.1.2}
REPOSITORY=${4:-aliyun/ilogtail}
PUSH=${5:-false}

HOST_OS=`uname -s`
ROOTDIR=$(cd $(dirname "${BASH_SOURCE[0]}") && cd .. && pwd)
GEN_DOCKERFILE=$GENERATED_HOME/Dockerfile

# automatically replace registery address to the fastest mirror
CN_REGION=sls-opensource-registry.cn-shanghai.cr.aliyuncs.com
US_REGION=sls-opensource-registry.us-east-1.cr.aliyuncs.com
cn_rtt=$(ping -c 3 -W 1 $CN_REGION | grep rtt | awk '{print $4}' | awk -F'/' '{print $2}' | awk -F '.' '{print $1}') || cn_rtt=99999
us_rtt=$(ping -c 3 -W 1 $US_REGION | grep rtt | awk '{print $4}' | awk -F'/' '{print $2}' | awk -F '.' '{print $1}') || us_rtt=99999
REG_REGION=$CN_REGION
if [[ "$us_rtt" -lt "$cn_rtt" ]]; then
  REG_REGION=$US_REGION
fi

mkdir -p $GENERATED_HOME
rm -rf $GEN_DOCKERFILE
touch $GEN_DOCKERFILE

if [[ $CATEGORY = "goc" || $CATEGORY = "build" ]]; then
    cat $ROOTDIR/docker/Dockerfile_$CATEGORY | grep -v "^#" | sed "s/$CN_REGION/$REG_REGION/" > $GEN_DOCKERFILE;
elif [[  $CATEGORY = "development" ]]; then
    cat $ROOTDIR/docker/Dockerfile_build | grep -v "^#" | sed "s/$CN_REGION/$REG_REGION/" > $GEN_DOCKERFILE;
    cat $ROOTDIR/docker/Dockerfile_development_part |grep -v "^#" | sed "s/$CN_REGION/$REG_REGION/" >> $GEN_DOCKERFILE;
elif [[  $CATEGORY = "production" ]]; then
    cat $ROOTDIR/docker/Dockerfile_production |grep -v "^#" | sed "s/$CN_REGION/$REG_REGION/" > $GEN_DOCKERFILE;
fi

echo "=============DOCKERFILE=================="
cat $GEN_DOCKERFILE
echo "========================================="
docker build --build-arg VERSION="$VERSION" \
 --build-arg HOST_OS="$HOST_OS" \
  -t "$REPOSITORY":"$VERSION" \
  --no-cache . -f $GEN_DOCKERFILE


if [[ $PUSH = "true" ]]; then
    echo "COMMAND:"
    echo "docker push $REPOSITORY:$VERSION"
    if [[ $VERSION = "latest" ]]; then
      echo "Current operation is so dangerous, you should push by yourself !!!"
    else
      docker push "$REPOSITORY:$VERSION"
    fi
fi
