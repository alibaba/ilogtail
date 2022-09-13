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

US_REGION="logtail-release-us-west-1.oss-us-west-1.aliyuncs.com"
CN_REGION="logtail-release-cn-beijing.oss-cn-beijing.aliyuncs.com"
REGION=$US_REGION

us_rtt=$(ping -c 1 -W 1 $US_REGION | grep rtt | awk '{print $4}' | awk -F'/' '{print $2}'|awk -F '.' '{print $1}')
cn_rtt=$(ping -c 1 -W 1 $CN_REGION | grep rtt | awk '{print $4}' | awk -F'/' '{print $2}'|awk -F '.' '{print $1}')

if [[ "$us_rtt" -gt "$cn_rtt" ]]; then
  REGION=$CN_REGION
fi

curl --connect-timeout 5 -m 10 https://$REGION/kubernetes/libebpf.so -o libebpf.so
