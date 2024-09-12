/*
 * Copyright 2024 iLogtail Authors
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
#include <curl/curl.h>

#include <cstdint>
#include <map>
#include <string>

#include "app_config/AppConfig.h"
#include "common/http/Curl.h"
#include "logger/Logger.h"
#include "common/http/SyncCurl.h"
#include "common/http/HttpResponse.h"

namespace logtail {
    bool Send(std::unique_ptr<HttpRequest>&& request, HttpResponse& response) {
        curl_slist* headers = NULL;
        CURL* curl = CreateCurlHandler(request->mMethod,
                                   request->mHTTPSFlag,
                                   request->mHost,
                                   request->mPort,
                                   request->mUrl,
                                   request->mQueryString,
                                   request->mHeader,
                                   request->mBody,
                                   response,
                                   headers,
                                   request->mTimeout,
                                   AppConfig::GetInstance()->IsHostIPReplacePolicyEnabled(),
                                   AppConfig::GetInstance()->GetBindInterface());
        if (curl == NULL) {
            LOG_ERROR(sLogger, ("failed to send request", "Init curl instance error")("request address", request.get()));
            return false;
        }
        bool success = false;
        while (++request->mTryCnt <= request->mMaxTryCnt) {
            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                long http_code = 0;
                if ((res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)) == CURLE_OK) {
                    response.mStatusCode = (int32_t)http_code;
                    success = true;
                    break;
                } else {
                    LOG_ERROR(sLogger, ("failed to send request", std::string("Request get info failed, curl error code : ") + curl_easy_strerror(res))("request address", request.get())("retryCnt", request->mTryCnt));
                }
            } else {
                LOG_ERROR(sLogger, ("failed to send request", std::string("Request operation failed, curl error code : ") + curl_easy_strerror(res))("request address", request.get())("retryCnt", request->mTryCnt));
            }
        } 
        if (headers != NULL) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);
        return success;
    }

} // namespace logtail
