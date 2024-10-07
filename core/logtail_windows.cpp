// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <Winsock2.h>
#include <direct.h>

#include <iostream>

#include "RuntimeUtil.h"
#include "application/Application.h"
#include "common/FileSystemUtil.h"
#include "common/Flags.h"
#include "common/JsonUtil.h"
#include "config/PipelineConfig.h"
#include "logger/Logger.h"
#include "monitor/LogtailAlarm.h"

using namespace logtail;

DECLARE_FLAG_STRING(loongcollector_lib_dir);
DECLARE_FLAG_STRING(loongcollector_config_dir);
DECLARE_FLAG_STRING(loongcollector_log_dir);
DECLARE_FLAG_STRING(loongcollector_data_dir);
DECLARE_FLAG_STRING(logtail_sys_conf_dir);
DECLARE_FLAG_STRING(check_point_filename);
DECLARE_FLAG_STRING(default_buffer_file_path);
DECLARE_FLAG_STRING(ilogtail_docker_file_path_config);
DECLARE_FLAG_STRING(metrics_report_method);
DECLARE_FLAG_INT32(data_server_port);
DECLARE_FLAG_BOOL(enable_env_ref_in_config);
DECLARE_FLAG_BOOL(enable_sls_metrics_format);
DECLARE_FLAG_BOOL(enable_containerd_upper_dir_detect);

static void overwrite_community_edition_flags() {
    // support run in installation dir on default
    STRING_FLAG(logtail_sys_conf_dir) = "../etc/";
    STRING_FLAG(check_point_filename) = "../data/checkpoint/logtail_check_point";
    STRING_FLAG(default_buffer_file_path) = "../data/checkpoint";
    STRING_FLAG(ilogtail_docker_file_path_config) = "../data/docker_path_config.json";
    STRING_FLAG(metrics_report_method) = "";
    INT32_FLAG(data_server_port) = 443;
    BOOL_FLAG(enable_env_ref_in_config) = true;
    BOOL_FLAG(enable_containerd_upper_dir_detect) = true;
    BOOL_FLAG(enable_sls_metrics_format) = false;
}

void do_worker_process() {
    std::string dir = GetProcessExecutionDir();

    Json::Value confJson(Json::objectValue);
    LoadConfigDetailFromFile(dir + "loongcollector_config.json", confJson, true);

#define PROCESSDIRFLAG(flag_name, env_name, dir_name) \
    LoadStringParameter(STRING_FLAG(flag_name), confJson, #flag_name, env_name); \
    if (STRING_FLAG(flag_name).empty()) { \
        STRING_FLAG(flag_name) = dir + "../" + #dir_name + "/"; \
    } else if (STRING_FLAG(flag_name).at(0) != '/') { \
        STRING_FLAG(flag_name) = dir + "../" + STRING_FLAG(flag_name) + "/"; \
    } \
    if (!CheckExistance(STRING_FLAG(flag_name))) { \
        if (Mkdirs(STRING_FLAG(flag_name))) { \
            std::cout << STRING_FLAG(flag_name) + " dir is not existing, create done" << std::endl; \
        } else { \
            std::cout << STRING_FLAG(flag_name) + " dir is not existing, create failed" << std::endl; \
            exit(0); \
        } \
    }

    PROCESSDIRFLAG(loongcollector_lib_dir, "ALIYUN_LOONGCOLLECTOR_LIB_DIR", lib);
    PROCESSDIRFLAG(loongcollector_config_dir, "ALIYUN_LOONGCOLLECTOR_CONFIG_DIR", etc);
    PROCESSDIRFLAG(loongcollector_log_dir, "ALIYUN_LOONGCOLLECTOR_LOG_DIR", log);
    PROCESSDIRFLAG(loongcollector_data_dir, "ALIYUN_LOONGCOLLECTOR_DATA_DIR", data);

    Logger::Instance().InitGlobalLoggers();

    // Initialize Winsock.
    int iResult;
    WSADATA wsaData;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        LOG_FATAL(sLogger, ("Initialize Winsock failed", iResult));
        return;
    }

    overwrite_community_edition_flags();

    Application::GetInstance()->Init();
    Application::GetInstance()->Start();
}

int main(int argc, char** argv) {
    gflags::SetUsageMessage(
        std::string("The Lightweight Collector of SLS in Alibaba Cloud\nUsage: ./ilogtail [OPTION]"));
    gflags::SetVersionString(std::string(ILOGTAIL_VERSION) + " Community Edition");
    google::ParseCommandLineFlags(&argc, &argv, true);

    do_worker_process();

    return 0;
}
