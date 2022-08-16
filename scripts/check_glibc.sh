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

set -ue
set -o pipefail

# intialize variables
OUT_DIR=${1:-output}
ROOTDIR=$(cd $(dirname "${BASH_SOURCE[0]}") && cd .. && pwd)
BIN="${ROOTDIR}/${OUT_DIR}/ilogtail"

# check if the symbols in ilogtail are compatible with GLIBC_2.5
awk_script=$(cat <<- EOF
NF>0 && \$(NF-1)~"GLIBC_" {
  sec=\$(NF-1);
  split(sec, a, "_");
  split(a[2], a, ".");
  ver=a[1]*1000+a[2];
  if (ver > 2005) {
    print "The following symbols are not compatible with GLIBC_2.5"
    print \$0
    exit 1
  }
}
EOF
)
objdump -T "$BIN" | awk "$awk_script"
