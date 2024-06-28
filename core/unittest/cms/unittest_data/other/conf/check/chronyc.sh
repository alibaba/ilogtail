#!/bin/bash

# See: https://aone.alibaba-inc.com/v2/project/794755/bug/50717222
if [ "$2" == "tracking" ]; then
  # $0 -c tracking
  echo B65C0C0B,182.92.12.11,3,1624589756.895936595,-0.000004705,-0.000005362,0.000013715,-36.415,-0.000,0.073,0.005060666,0.000683602,64.1,Normal
else
  # $0 -n sources
  echo '*'
fi
