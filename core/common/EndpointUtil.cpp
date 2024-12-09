/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/EndpointUtil.h"

#include "logger/Logger.h"

using namespace std;

namespace logtail {

bool IsHttpsEndpoint(const string& endpoint) {
    return endpoint.find("https://") == 0;
}

string StandardizeEndpoint(const string& endpoint) {
    auto bpos = endpoint.find("://");
    if (bpos == string::npos) {
        bpos = 0;
    } else {
        bpos += strlen("://");
    }

    auto epos = endpoint.find("/", bpos);
    if (epos == string::npos) {
        epos = endpoint.length();
    }
    return endpoint.substr(bpos, epos - bpos);
}

string GetHostFromEndpoint(const std::string& endpoint) {
    static size_t httpSchemaLen = strlen("http://"), httpsSchemaLen = strlen("https://");
    if (endpoint.find("https://") == 0) {
        return endpoint.substr(httpsSchemaLen);
    } else if (endpoint.find("http://") == 0) {
        return endpoint.substr(httpSchemaLen);
    } else {
        return endpoint;
    }
}

} // namespace logtail
