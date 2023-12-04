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
    
string StandardizeEndpoint(const string& endpoint, const string& defaultEndpoint) {
    string res = endpoint;
    if (endpoint.find("https://") == 0) {
        if (endpoint.size() < string("https://x").size()) {
            LOG_WARNING(sLogger, ("invalid endpoint", endpoint)("use default instead", defaultEndpoint));
            return defaultEndpoint;
        }
    } else if (endpoint.find("http://") == 0) {
        if (endpoint.size() < string("http://x").size()) {
            LOG_WARNING(sLogger, ("invalid endpoint", endpoint)("use default instead", defaultEndpoint));
            return defaultEndpoint;
        }
    } else {
        res = string("http://") + endpoint;
        LOG_DEBUG(sLogger, ("add default protocol for endpoint, old", endpoint)("new", res));
    }
    // trim the last '/'
    if (res[res.size() - 1] == '/') {
        return res.substr(0, res.size() - 1);
    }
    return res;
}

} // namespace logtail
