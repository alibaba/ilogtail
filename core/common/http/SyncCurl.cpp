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

#pragma once

#include <curl/curl.h>

#include <cstdint>
#include <map>
#include <string>

#include "common/http/SyncCurl.h"
#include "common/http/HttpResponse.h"

namespace logtail {
    bool Send(unique_ptr<HttpRequest>&& request, HttpResponse& response) {
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
            //throw LOGException(LOGE_UNKNOWN_ERROR, "Init curl instance error.");
            LOG_ERROR(sLogger, ("failed to send request", "failed to init curl handler")("request address", request.get()));
            return false;
        }

        CURLcode res = curl_easy_perform(curl);
        if (headers != NULL) {
            curl_slist_free_all(headers);
        }

        switch (res) {
            case CURLE_OK:
                break;
            case CURLE_OPERATION_TIMEDOUT:
                curl_easy_cleanup(curl);
                //throw LOGException(LOGE_CLIENT_OPERATION_TIMEOUT, "Request operation timeout.");
                return false;
            case CURLE_COULDNT_CONNECT:
                curl_easy_cleanup(curl);
                //throw LOGException(LOGE_REQUEST_TIMEOUT, "Can not connect to server.");
                return false;
            default:
                curl_easy_cleanup(curl);
                //throw LOGException(LOGE_REQUEST_ERROR,
                //                   string("Request operation failed, curl error code : ") + curl_easy_strerror(res));
                return false;
        }

        long http_code = 0;
        if ((res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK) {
            curl_easy_cleanup(curl);
            //throw LOGException(LOGE_UNKNOWN_ERROR,
            //                   string("Get curl response code error, curl error code : ") + curl_easy_strerror(res));
            return false;
        }
        response.mStatusCode = (int32_t)http_code;
        curl_easy_cleanup(curl);
        return true;
    }

} // namespace logtail
