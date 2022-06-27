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


VERSION=${1:-1.1.0}
DOCKER_TYPE=$2
REPOSITORY=${3:-aliyun/ilogtail}
PUSH=${4:-false}

HOST_OS=`uname -s`
ROOTDIR=$(cd $(dirname "${BASH_SOURCE[0]}") && cd .. && pwd)
GEN_DOCKERFILE_HOME="gen_dockerfile_home"
GEN_DOCKERFILE=$GEN_DOCKERFILE_HOME/Dockerfile

case $DOCKER_TYPE in
plugin) REPOSITORY=${REPOSITORY}-plugin;;
core) REPOSITORY=${REPOSITORY}-core;;
*) REPOSITORY=${REPOSITORY};;
esac

rm -rf $GEN_DOCKERFILE_HOME && mkdir $GEN_DOCKERFILE_HOME && touch $GEN_DOCKERFILE

if [[ $DOCKER_TYPE = "core" || $DOCKER_TYPE = "plugin" || $DOCKER_TYPE = "plugin_base" || $DOCKER_TYPE = "goc" ]]; then
    cat $ROOTDIR/docker/Dockerfile_$DOCKER_TYPE > $GEN_DOCKERFILE;
elif [ $DOCKER_TYPE = "default" ]; then
    cat $ROOTDIR/docker/Dockerfile_core > $GEN_DOCKERFILE;
    cat $ROOTDIR/docker/Dockerfile_plugin >> $GEN_DOCKERFILE;
    cat $ROOTDIR/docker/Dockerfile >> $GEN_DOCKERFILE;
elif [ $DOCKER_TYPE = "e2e" ]; then
    cat $ROOTDIR/docker/Dockerfile_core > $GEN_DOCKERFILE;
    cat $ROOTDIR/docker/Dockerfile_plugin_coverage >> $GEN_DOCKERFILE;
    cat $ROOTDIR/docker/Dockerfile >> $GEN_DOCKERFILE;
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
