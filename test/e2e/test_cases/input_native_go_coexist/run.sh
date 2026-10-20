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

# Wait for loongcollector to start watching both files, then append a
# distinct live line to each so both input_file pipelines must fire.
sleep 8
echo "file-a-live=====" >> /root/a.log
echo "file-b-live=====" >> /root/b.log
# Keep the sidecar alive so compose does not treat it as a failed one-shot.
sleep 3600
