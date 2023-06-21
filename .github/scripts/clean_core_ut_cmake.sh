#!/bin/bash
# Copyright 2023 iLogtail Authors
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

TARGET_ARTIFACT_PATH=${TARGET_ARTIFACT_PATH:-"./output/unittest"}

search_files() {
    for file in "$1"/*; do
        if [ -d "$file" ]; then
            # Recursively handle folder
            search_files "$file"
        elif [[ -f "$file" ]]; then
            extension="${file##*.}"
            unittest="${file##*_}"
            if [ "$unittest" == "unittest" ]; then
                echo "Strip executable file $file"
                strip "$file"
            elif [[ "$extension" == "a" || "$extension" == "o" ]]; then
                rm "$file"
            fi
        fi
    done
}

search_files $TARGET_ARTIFACT_PATH
