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

# There are 2 kinds of test, which are e2e and performance.
TYPE=$1
TEST_SCOPE=$2

ROOT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && cd .. && pwd)
TESTDIR=$ROOT_DIR/test

cd "$TEST_HOME"
if [ "$TEST_SCOPE" = "core" ]; then
  go test -v -run ^TestE2EOnDockerComposeCore$ github.com/alibaba/ilogtail/test/$TYPE
else if [ "$TEST_SCOPE" = "performance" ]; then
  go test -v -run ^TestE2EOnDockerComposePerformance$ github.com/alibaba/ilogtail/test/$TYPE
else
  go test -v -run ^TestE2EOnDockerCompose$ github.com/alibaba/ilogtail/test/$TYPE
fi

if [ $? = 0 ]; then
  sh "$ROOT_DIR"/scripts/e2e_coverage.sh "$TYPE"
  echo "========================================="
  echo "All testing cases are passed"
  echo "========================================="
else
  exit 1
fi
