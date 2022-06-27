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

export CATEGORY=$1
export VERSION=${2:-1.1.0}
export REPOSITORY=${3:-aliyun/ilogtail}

BUILD_SCRIPT_FILE=gen_build.sh
COPY_SCRIPT_FILE=gen_copy.sh

BINDIR=$ROOTDIR/bin

function generateBuildScript() {
  rm -rf $BUILD_SCRIPT_FILE && touch $BUILD_SCRIPT_FILE && chmod 755 $BUILD_SCRIPT_FILE
  if [ $CATEGORY = "plugin" ]; then
    echo './scripts/plugin_build.sh vendor c-shared' >> gen_build.sh;
  elif [ $CATEGORY = "core" ]; then
    echo 'mkdir -p core/build && cd core/build && cmake -D LOGTAIL_VERSION=${VERSION} .. && make -sj32' >> gen_build.sh;
  elif [ $CATEGORY = "all" ]; then
    echo './scripts/plugin_build.sh vendor c-shared && mkdir -p core/build && cd core/build && cmake -D LOGTAIL_VERSION=${VERSION} .. && make -sj32' >> gen_build.sh;
  elif [ $CATEGORY = "e2e" ]; then
    echo './scripts/plugin_gocbuild.sh && mkdir -p core/build && cd core/build && cmake -D LOGTAIL_VERSION=${VERSION} .. && make -sj32' >> gen_build.sh;
  fi
}



function generateCopyScript() {
  rm -rf $COPY_SCRIPT_FILE && touch $COPY_SCRIPT_FILE && chmod 755 $COPY_SCRIPT_FILE
  echo 'BINDIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)/bin/' >> $COPY_SCRIPT_FILE
  echo 'rm -rf $BINDIR && mkdir $BINDIR' >> $COPY_SCRIPT_FILE
  echo "id=\$(docker create ${REPOSITORY}:${VERSION})" >> $COPY_SCRIPT_FILE

  if [ $CATEGORY = "plugin" ]; then
    echo ' docker cp "$id":/src/bin/libPluginBase.so $BINDIR'  >> $COPY_SCRIPT_FILE;
  elif [ $CATEGORY = "core" ]; then
    echo 'docker cp "$id":/src/core/build/ilogtail $BINDIR'  >> $COPY_SCRIPT_FILE;
    echo 'docker cp "$id":/src/core/build/plugin/libPluginAdapter.so $BINDIR'  >> $COPY_SCRIPT_FILE;
  else
    echo ' docker cp "$id":/src/bin/libPluginBase.so $BINDIR'  >> $COPY_SCRIPT_FILE;
    echo 'docker cp "$id":/src/core/build/ilogtail $BINDIR'  >> $COPY_SCRIPT_FILE;
    echo 'docker cp "$id":/src/core/build/plugin/libPluginAdapter.so $BINDIR'  >> $COPY_SCRIPT_FILE;
  fi
  echo 'docker rm -v "$id"' >> $COPY_SCRIPT_FILE;
}



generateBuildScript
generateCopyScript





