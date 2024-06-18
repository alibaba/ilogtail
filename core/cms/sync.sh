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
    # 打印120个-
    printf "\n%120s\n" | tr " " "-"

    printf "%s " "$@"
    echo

    # echo "${all_params}"
    eval $@
}

# common
# -u skip files that are newer on the receiver
run_cmd rsync -auv --exclude="impl/*_test.cpp" ${ARGUS_SRC}/common ${ONE_AGENT_SRC}
# --include='*/' ：首先包括所有目录，这样 rsync 就能够递归进入子目录。
# --include='*.txt' ：然后包括所有的 .txt 文件。
# --exclude='*' ：最后排除所有其他文件。
run_cmd rsync -auv --include='*/' --include="impl/*_test.cpp" --exclude="*" ${ARGUS_SRC}/common ${ONE_AGENT_UT}

