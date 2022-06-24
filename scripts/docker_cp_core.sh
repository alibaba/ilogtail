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
REPOSITORY=${2:-aliyun/ilogtail}
ROOTDIR=$(cd $(dirname "${BASH_SOURCE[0]}") && cd .. && pwd)

[ -d "$ROOTDIR"/build ] && rm -rf "$ROOTDIR"/build || :
mkdir -p "${ROOTDIR}/bin"

id=$(docker create ${REPOSITORY}:${VERSION})
echo "$id"
docker cp "$id":/src/core/build "$ROOTDIR"
docker rm -v "$id"
cp "${ROOTDIR}/build/ilogtail" "${ROOTDIR}/bin/"
cp "${ROOTDIR}/build/plugin/libPluginAdapter.so" "${ROOTDIR}/bin/"