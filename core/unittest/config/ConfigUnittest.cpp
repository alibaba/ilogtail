// Copyright 2023 iLogtail Authors
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

#include <memory>
#include <string>

#include "json/json.h"

#include "common/JsonUtil.h"
#include "config/NewConfig.h"
#include "plugin/PluginRegistry.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class ConfigUnittest : public testing::Test {
public:
    void HandleValidConfig() const;
    void HandleInvalidCreateTime() const;
    void HandleInvalidGlobal() const;
    void HandleInvalidInputs() const;
    void HandleInvalidProcessors() const;
    void HandleInvalidAggregators() const;
    void HandleInvalidFlushers() const;
    void HandleInvalidExtensions() const;
    void TestReplaceEnvVarRef() const;

protected:
    static void SetUpTestCase() { PluginRegistry::GetInstance()->LoadPlugins(); }
    static void TearDownTestCase() { PluginRegistry::GetInstance()->UnloadPlugins(); }

private:
    const string configName = "test";
};

void ConfigUnittest::HandleValidConfig() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    configStr = R"(
        {
            "createTime": 123456789,
            "global": {
                "TopicType": "none"
            },
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_json"
                }
            ],
            "aggregators":[
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ],
            "extensions": [
                {
                    "Type": "ext_basicauth"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(configName, config->mName);
    APSARA_TEST_EQUAL(123456789, config->mCreateTime);
    APSARA_TEST_NOT_EQUAL(config->mGlobal, nullptr);
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(2, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mAggregators.size());
    APSARA_TEST_EQUAL(2, config->mFlushers.size());
    APSARA_TEST_EQUAL(1, config->mExtensions.size());

    // topology 1: native -> native -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_FALSE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_FALSE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_FALSE(config->HasGoPlugin());

    // topology 2: extended -> native -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 3: (native, extended) -> native -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 4: native -> extended -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 5: extended -> extended -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_FALSE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 6: (native, extended) -> extended -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 7: native -> (native -> extended) -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(2, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 8: extended -> (native -> extended) -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 9: (native, extended) -> (native -> extended) -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 10: native -> none -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(0, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_FALSE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_FALSE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_FALSE(config->HasGoPlugin());

    // topology 11: extended -> none -> native (future changes maybe applied)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(0, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_FALSE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 12: (native, extended) -> none -> native
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 13: native -> native -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_FALSE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 14: extended -> native -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 15: (native, extended) -> native -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 16: native -> extended -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 17: extended -> extended -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_FALSE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 18: (native, extended) -> extended -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 19: native -> (native -> extended) -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(2, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 20: extended -> (native -> extended) -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 21: (native, extended) -> (native -> extended) -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 22: native -> none -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(0, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_FALSE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 23: extended -> none -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(0, config->mProcessors.size());
    APSARA_TEST_EQUAL(1, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_FALSE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 24: (native, extended) -> none -> extended
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 25: native -> native -> (native, extended) (future changes maybe applied)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(2, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 26: extended -> native -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 27: (native, extended) -> native -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 28: native -> extended -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(2, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 29: extended -> extended -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(1, config->mProcessors.size());
    APSARA_TEST_EQUAL(2, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_FALSE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 30: (native, extended) -> extended -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 31: native -> (naive -> extended) -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_json"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(2, config->mProcessors.size());
    APSARA_TEST_EQUAL(2, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 32: extended -> (native -> extended) -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 33: (native, extended) -> (native -> extended) -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_regex"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // topology 34: native -> none -> (native, extended) (future changes maybe applied)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(0, config->mProcessors.size());
    APSARA_TEST_EQUAL(2, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_TRUE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 35: extended -> none -> (native, extended) (future changes maybe applied)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(1, config->mInputs.size());
    APSARA_TEST_EQUAL(0, config->mProcessors.size());
    APSARA_TEST_EQUAL(2, config->mFlushers.size());
    APSARA_TEST_TRUE(config->ShouldNativeFlusherConnectedByGoPipeline());
    APSARA_TEST_TRUE(config->IsFlushingThroughGoPipelineExisted());
    APSARA_TEST_FALSE(config->IsProcessRunnerInvolved());
    APSARA_TEST_TRUE(config->HasGoPlugin());

    // topology 36: (native, extended) -> none -> (native, extended)
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                },
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());
}

void ConfigUnittest::HandleInvalidCreateTime() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    configStr = R"(
        {
            "createTime": "123456789",
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_TRUE(config->Parse());
    APSARA_TEST_EQUAL(0, config->mCreateTime);
}

void ConfigUnittest::HandleInvalidGlobal() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    // global is not of type object
    configStr = R"(
        {
            "global": []
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());
}

void ConfigUnittest::HandleInvalidInputs() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    // no inputs
    configStr = R"(
        {
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // inputs is not of type array
    configStr = R"(
        {
            "inputs": {},
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // inputs is empty
    configStr = R"(
        {
            "inputs": [],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // inputs element is not of type object
    configStr = R"(
        {
            "inputs": [
                []
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // no Type
    configStr = R"(
        {
            "inputs": [
                {
                    "type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // Type is not of type string
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": true
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // unsupported input
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_unknown"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "input_unknown"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                },
                {
                    "Type": "input_unknown"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // more than 1 native input
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // native and extended inputs coexist
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                },
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdoutinput_file"
                },
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());
}

void ConfigUnittest::HandleInvalidProcessors() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    // processors is not of type array
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": {}
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // processors element is not of type object
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                []
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // no Type
    configStr = R"(
        {
            "inputs": [
                {
                    "type": "input_file"
                }
            ],
            "processors": [
                {
                    "type": "processor_parse_regex_native"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // Type is not of type string
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": true
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // unsupported processors
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_unknown"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_unknown"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_unknown"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // native processor plugin comes after extended processor plugin
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_json"
                },
                {
                    "Type": "processor_parse_regex_native"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // native processor plugins coexist with extended input plugins
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "service_docker_stdout"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // native processor plugins coexist with input_observer_network
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_observer_network"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // native processor plugins coexist with processor_spl
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_parse_regex_native"
                },
                {
                    "Type": "processor_spl"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "processors": [
                {
                    "Type": "processor_spl"
                },
                {
                    "Type": "processor_parse_regex_native"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());
}

void ConfigUnittest::HandleInvalidAggregators() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    // aggregators is not of type array
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "aggregators": {},
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // aggregators element is not of type object
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "aggregators": [
                []
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // no Type
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "aggregators": [
                {
                    "type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // Type is not of type string
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "aggregators": [
                {
                    "Type": true
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // unsupported aggregator
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_unknown"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // more than 1 aggregator
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                },
                {
                    "Type": "aggregator_default"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_kafka_v2"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // aggregator plugins exist in native flushing mode
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "aggregators": [
                {
                    "Type": "aggregator_context"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());
}

void ConfigUnittest::HandleInvalidFlushers() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    // no flushers
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // flushers is not of type array
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": {}
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // flushers is empty
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": []
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // flushers element is not of type object
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                []
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // no Type
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // Type is not of type string
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "type": "flusher_sls"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // unsupported flusher
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_unknown"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());
}

void ConfigUnittest::HandleInvalidExtensions() const {
    Json::Value configJson;
    string configStr, errorMsg;
    unique_ptr<NewConfig> config;

    // extensions is not of type array
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ],
            "extensions": {}
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // extensions element is not of type object
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ],
            "extensions": [
                []
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // no Type
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ],
            "extensions": [
                {
                    "type": "ext_basicauth"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // Type is not of type string
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ],
            "extensions": [
                {
                    "type": true
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // unsupported extension
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ],
            "extensions": [
                {
                    "Type": "ext_unknown"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());

    // extension plugins exist when no extended plugin is given
    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ],
            "extensions": [
                {
                    "Type": "ext_basicauth"
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    APSARA_TEST_FALSE(config->Parse());
}

void ConfigUnittest::TestReplaceEnvVarRef() const {
#if defined(__linux__)
    setenv("__path4ut", "_home/$work", 1);
    setenv("__file4ut", "!transaction/~un-do.log", 1);
#else
    _putenv_s("__path4ut", "_home/$work");
    _putenv_s("__file4ut", "!transaction/~un-do.log");
#endif

    Json::Value configJson, resJson;
    string configStr, resStr, errorMsg;
    unique_ptr<NewConfig> config;

    configStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file",
                    "FilePaths": [
                        "/${__path4ut}/${__file4ut}"
                    ]
                }
            ]
        }
    )";

    resStr = R"(
        {
            "inputs": [
                {
                    "Type": "input_file",
                    "FilePaths": [
                        "/_home/$work/!transaction/~un-do.log"
                    ]
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseConfig(configStr, configJson, errorMsg));
    APSARA_TEST_TRUE(ParseConfig(resStr, resJson, errorMsg));
    config.reset(new NewConfig(configName, std::move(configJson)));
    config->ReplaceEnvVar();
    APSARA_TEST_TRUE(config->mDetail == resJson);
}

UNIT_TEST_CASE(ConfigUnittest, HandleValidConfig)
UNIT_TEST_CASE(ConfigUnittest, HandleInvalidCreateTime)
UNIT_TEST_CASE(ConfigUnittest, HandleInvalidGlobal)
UNIT_TEST_CASE(ConfigUnittest, HandleInvalidInputs)
UNIT_TEST_CASE(ConfigUnittest, HandleInvalidProcessors)
UNIT_TEST_CASE(ConfigUnittest, HandleInvalidAggregators)
UNIT_TEST_CASE(ConfigUnittest, HandleInvalidFlushers)
UNIT_TEST_CASE(ConfigUnittest, HandleInvalidExtensions)
UNIT_TEST_CASE(ConfigUnittest, TestReplaceEnvVarRef)

} // namespace logtail

UNIT_TEST_MAIN
