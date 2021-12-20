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


VERSION=$1
DOCKER_TYPE=$2

HOST_OS=`uname -s`

case $DOCKER_TYPE in
coverage) DOCKERFILE=Dockerfile_coverage;;
base) DOCKERFILE=Dockerfile_base;;
lib) DOCKERFILE=Dockerfile_lib;;
whole) DOCKERFILE=Dockerfile_whole;;
*) DOCKERFILE=Dockerfile;;
esac

docker build --build-arg VERSION="$VERSION" \
 --build-arg HOST_OS="$HOST_OS" \
  -t aliyun/ilogtail:"$VERSION" \
  --no-cache . -f docker/$DOCKERFILE

