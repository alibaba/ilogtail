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
REPOSITORY=${2:-aliyun/ilogtail}
CATEGORY=${3:-core}

ROOTDIR=$(cd $(dirname "${BASH_SOURCE[0]}") && cd .. && pwd)
mkdir -p "$ROOTDIR"/bin

case $CATEGORY in
plugin) REPOSITORY=${REPOSITORY}-plugin;;
core) REPOSITORY=${REPOSITORY}-core;;
*) REPOSITORY=${REPOSITORY};;
esac


id=$(docker create ${REPOSITORY}:${VERSION})
echo "$id"

if [ "$CATEGORY" = "core" ]; then
  docker cp "$id":/src/core/build/ilogtail "${ROOTDIR}/bin/"
  docker cp "$id":/src/core/build/plugin/libPluginAdapter.so "${ROOTDIR}/bin/"
else
  docker cp "$id":/src/bin/libPluginBase.so "${ROOTDIR}/bin/"
  docker cp "$id":/src/bin/libPluginBase.h "${ROOTDIR}/bin/"
fi

docker rm -v "$id"
