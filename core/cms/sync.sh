#!/bin/bash

# 锚定基准目录
if [ -z "${SHELL_FOLDER}" ]; then
    SHELL_FOLDER=$(dirname "$(readlink -f "$0")")
fi

# Argus
ARGUS_ROOT=/workspaces/argusagent
ARGUS_SRC=${ARGUS_ROOT}/src

# OneAgent
ONE_AGENT_SRC=${SHELL_FOLDER}
if [ -z "${TEST_DIR}" ]; then
    TEST_DIR="../unittest/cms"
fi
ONE_AGENT_UT="${SHELL_FOLDER}/${TEST_DIR}"
if [ ! -f "${ONE_AGENT_UT}" ]; then
    mkdir -p "${ONE_AGENT_UT}"
fi

echo "ONE_AGENT_SRC : ${ONE_AGENT_SRC}"
echo "ONE_AGENT_UT  : ${ONE_AGENT_UT}"
echo "ARGUSAGENT_SRC: ${ARGUS_SRC}"

# exit 0
function run_cmd()
{
    echo
    # # 打印120个-
    # printf "\n%100s\n" | tr " " "-"

    printf "%s " "$@"
    echo

    # echo "${all_params}"
    eval $@
}

echo

# tail -n +2 : 忽略rsync的第一行
# head -n -3 : 忽略rsync最后两行的统计信息
# 上面两行的效果：仅显示同步的文件

# common
# -u skip files that are newer on the receiver
# run_cmd 
rsync -auv --exclude=CMakeLists.txt --exclude="impl/*_test.cpp" ${ARGUS_SRC}/common ${ONE_AGENT_SRC} | tail -n +2 | head -n -3
# --include='*/' ：首先包括所有目录，这样 rsync 就能够递归进入子目录。
# --include='*.txt' ：然后包括所有的 .txt 文件。
# --exclude='*' ：最后排除所有其他文件。
# run_cmd 
rsync -auv --include='*/' --include="impl/*_test.cpp" --exclude="*" ${ARGUS_SRC}/common ${ONE_AGENT_UT} | tail -n +2 | head -n -3

# third_party
# run_cmd 
rsync -auv ${ARGUS_ROOT}/third_party/fmt ${ONE_AGENT_SRC}/third_party | tail -n +2 | head -n -3
# run_cmd 
rsync -auv --exclude=WinHttpClient ${ARGUS_ROOT}/third_party/header_only ${ONE_AGENT_SRC}/third_party | tail -n +2 | head -n -3

# sic
# run_cmd
rsync -auv --exclude=CMakeLists.txt --exclude="tests" ${ARGUS_SRC}/sic ${ONE_AGENT_SRC} | tail -n +2 | head -n -3
# run_cmd
rsync -auv --exclude={CMakeLists.txt,"tests/conf","src","*.h"} --include="*_test.cpp" ${ARGUS_SRC}/sic ${ONE_AGENT_UT} | tail -n +2 | head -n -3

# cloudMonitor, detect, core
# https://www.ruanyifeng.com/blog/2020/08/rsync.html
# run_cmd
rsync -auv --include='*/' --exclude={CMakeLists.txt,README.md,"*_test.cpp","sls_*.*"} ${ARGUS_SRC}/cloudMonitor ${ARGUS_SRC}/detect ${ARGUS_SRC}/core ${ONE_AGENT_SRC} | tail -n +2 | head -n -3
# run_cmd
rsync -auv --include='*/' --exclude="sls_*.*" --include="*_test.cpp" --exclude="*"    ${ARGUS_SRC}/cloudMonitor ${ARGUS_SRC}/detect ${ARGUS_SRC}/core ${ONE_AGENT_UT} | tail -n +2 | head -n -3

#
# run_cmd
rsync -auv --exclude={'tmp/*','local_data/logs/*','GPU/.gpu'} ${ARGUS_ROOT}/test/unit_test/conf ${ONE_AGENT_UT}/unittest_data/other | tail -n +2 | head -n -3
# run_cmd
rsync -auv --exclude='conf/logs/*' ${ARGUS_ROOT}/src/sic/tests/conf ${ONE_AGENT_UT}/unittest_data/sic | tail -n +2 | head -n -3

if [ ! -f "${ONE_AGENT_SRC}/runtime" ]; then
    mkdir -p "${ONE_AGENT_SRC}/runtime"
fi
# run_cmd
rsync -auv ${ARGUS_ROOT}/cmd/release_cloud_monitor/moduleTask.json ${ONE_AGENT_SRC}/runtime/moduleTask.json | tail -n +2 | head -n -3
# run_cmd
rsync -auv ${ARGUS_ROOT}/cmd/release_cloud_monitor/agent.properties ${ONE_AGENT_SRC}/runtime/agent.properties | tail -n +2 | head -n -3
