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


VERSION=${1:-github-latest}
DOCKER_TYPE=$2
REPOSITORY=${3:-aliyun/ilogtail}
PUSH=${4:-false}

HOST_OS=`uname -s`

case $DOCKER_TYPE in
core) DOCKERFILE=Dockerfile_core;;
coverage) DOCKERFILE=Dockerfile_coverage;;
base) DOCKERFILE=Dockerfile_base;;
lib) DOCKERFILE=Dockerfile_lib;;
whole) DOCKERFILE=Dockerfile_whole;;
*) DOCKERFILE=Dockerfile;;
esac

docker build --build-arg VERSION="$VERSION" \
 --build-arg HOST_OS="$HOST_OS" \
  -t "$REPOSITORY":"$VERSION" \
  --no-cache . -f docker/$DOCKERFILE


if [[ $PUSH = "true" ]]; then
    echo "COMMAND:"
    echo "docker push $REPOSITORY:$VERSION"
    if [[ $VERSION = "latest" ]]; then
      echo "Current operation is so dangerous, you should push by yourself !!!"
    else
      docker push "$REPOSITORY:$VERSION"
    fi
fi
