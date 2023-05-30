/*
 * Copyright 2022 iLogtail Authors
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
#include "sdk/Common.h"
#include <list>
#include <memory>
#include <chrono>
#include <vector>
#include <set>
#include <map>

namespace logtail {
static const char* emptySHA256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
static const char* signPrefix = "HMAC-SHA256";
static const char* authorization = "Authorization";

static const char* v4Date = "X-Date";
static const char* v4ContentSHA256 = "X-Content-Sha256";
static const char* v4SecurityToken = "X-Security-Token";
static const char* iso8601Layout = "%Y%m%dT%H%M%SZ";
static const char* yyyyMMdd = "%Y%m%d";
static const std::set<char> nonEscape
    = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
       'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
       's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_', '.', '~'};

struct MetaData {
    std::string algorithm;
    std::string credentialScope;
    std::string signedHeaders;
    std::string date;
    std::string region;
    std::string service;
    MetaData(std::string algorithm,
             std::string credentialScope,
             std::string signedHeaders,
             std::string date,
             std::string service,
             std::string region) {
        this->algorithm = algorithm;
        this->credentialScope = credentialScope;
        this->signedHeaders = signedHeaders;
        this->date = date;
        this->region = region;
        this->service = service;
    }
};

class BytedanceSignV4 {
public:
    BytedanceSignV4() = default;
    ~BytedanceSignV4() = default;

    void signHeader(sdk::AsynRequest& request);
    static std::string uriEncode(const std::string& in, bool encodeSlash);
    static std::string encodeQuery(std::map<std::string, std::string> query);

    std::string accessKeyId;
    std::string secretAccessKey;
    std::string securityToken;
    std::string service;
    std::string region;

private:
    static void prepareRequestV4(sdk::AsynRequest& request);
    static std::string hashedCanonicalRequestV4(sdk::AsynRequest& request, MetaData& metaData);
    static std::string
    stringToSignV4(const sdk::AsynRequest& request, const std::string& hashedCanonReq, MetaData& metaData);
    static std::string signatureV4(const std::string& sk,
                                   const std::string& date,
                                   const std::string& region,
                                   const std::string& service,
                                   const std::string& stringToSign);
    static std::string buildAuthHeaderV4(const std::string signature, const MetaData& metaData, const std::string ak);
    static std::string encodePath(const std::string& path);
};
} // namespace logtail