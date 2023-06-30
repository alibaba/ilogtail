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

#include "VolcengineSign.h"
#include <ctime>
#include <utility>
#include <vector>
#include <sstream>
#include <algorithm>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <cstring>
#include <thread>

namespace logtail {
std::string joinMapToString(const std::vector<std::pair<std::string, std::string>>& KVs) {
    std::vector<std::string> keys(KVs.size());
    for (int i = 0; i < KVs.size(); ++i) {
        keys[i] = KVs[i].first;
    }
    std::sort(keys.begin(), keys.end());
    std::string ret;
    for (auto& key : keys) {
        if (key != keys[0]) {
            ret.append(";");
        }
        ret.append(key);
    }
    return ret;
}

std::string transTimeToFormat(const time_t& t, const char* format) {
    char timebuf[32] = {'\0'};
    std::tm gmttm;
#ifdef _WIN32
    gmtime_s(&gmttm, &t);
#else
    gmtime_r(&t, &gmttm);
#endif
    std::strftime(timebuf, sizeof(timebuf), format, &gmttm);
    std::string ret(timebuf);
    return ret;
}

std::string stringToHex(const unsigned char* input, int length) {
    static const char hex_digits[] = "0123456789abcdef";
    std::string output;
    for (int i = 0; i < length; ++i) {
        output.push_back(hex_digits[input[i] >> 4]);
        output.push_back(hex_digits[input[i] & 15]);
    }
    return output;
}

std::string toLower(const std::string& input) {
    char* inp = (char*)input.c_str();
    std::stringstream ret;
    for (int i = 0; i < input.length(); ++i) {
        if (inp[i] >= 'A' && inp[i] <= 'Z') {
            char low = inp[i] + 32;
            ret << low;
        } else {
            ret << inp[i];
        }
    }
    return ret.str();
}

bool compareByPairKey(const std::pair<std::string, std::string>& dataA,
                      const std::pair<std::string, std::string>& dataB) {
    return dataA.first < dataB.first;
}

void VolcengineSignV4::SignHeader(sdk::AsynRequest& request) {
    if (strcmp(this->accessKeyId.c_str(), "") == 0 || strcmp(this->secretAccessKey.c_str(), "") == 0
        || strcmp(this->securityToken.c_str(), "") == 0 || strcmp(this->service.c_str(), "") == 0) {
        // Todo log
        return;
    }
    prepareRequestV4(request);
    MetaData metaData("", "", "", "", this->service, this->region);
    request.mHeader["X-Security-Token"] = this->securityToken;
    // sign：step 1
    std::string hashedCanonReq = hashedCanonicalRequestV4(request, metaData);
    // sign：step 2
    std::string stringToSign = stringToSignV4(request, hashedCanonReq, metaData);
    // sign：step 3
    std::string signature
        = signatureV4(this->secretAccessKey, metaData.date, metaData.region, metaData.service, stringToSign);

    request.mHeader["Authorization"] = buildAuthHeaderV4(signature, metaData, this->accessKeyId);
}

std::string VolcengineSignV4::buildAuthHeaderV4(const std::string signature, const MetaData& metaData, const std::string ak) {
    std::string auth;
    auth.append(metaData.algorithm)
        .append(" Credential=")
        .append(ak)
        .append("/")
        .append(metaData.credentialScope)
        .append(", SignedHeaders=")
        .append(metaData.signedHeaders)
        .append(", Signature=")
        .append(signature);
    return auth;
}

std::string VolcengineSignV4::signatureV4(const std::string& sk,
                                const std::string& date,
                                const std::string& region,
                                const std::string& service,
                                const std::string& stringToSign) {
    unsigned int mdLen = 32;
    unsigned char unsignedDate[32];
    HMAC(EVP_sha256(),
         sk.c_str(),
         sk.size(),
         reinterpret_cast<const unsigned char*>(date.c_str()),
         date.size(),
         unsignedDate,
         &mdLen);
    unsigned char unsignedRegion[32];
    HMAC(EVP_sha256(),
         unsignedDate,
         32,
         reinterpret_cast<const unsigned char*>(region.c_str()),
         region.size(),
         unsignedRegion,
         &mdLen);
    unsigned char unsignedService[32];
    HMAC(EVP_sha256(),
         unsignedRegion,
         32,
         reinterpret_cast<const unsigned char*>(service.c_str()),
         service.size(),
         unsignedService,
         &mdLen);
    unsigned char signingKey[32];
    HMAC(EVP_sha256(), unsignedService, 32, reinterpret_cast<const unsigned char*>("request"), 7, signingKey, &mdLen);

    unsigned char sig[32];
    HMAC(EVP_sha256(),
         signingKey,
         32,
         reinterpret_cast<const unsigned char*>(stringToSign.c_str()),
         stringToSign.size(),
         sig,
         &mdLen);
    std::string signature = stringToHex(sig, 32);
    return signature;
}

std::string
VolcengineSignV4::stringToSignV4(const sdk::AsynRequest& request, const std::string& hashedCanonReq, MetaData& metaData) {
    std::string requestTs;
    auto it = request.mHeader.find("X-Date");
    if (it != request.mHeader.end()) {
        requestTs = it->second;
    }

    metaData.algorithm = signPrefix;
    metaData.date = requestTs.substr(0, 8);
    std::string credentialScope;
    metaData.credentialScope = credentialScope.append(metaData.date)
                                   .append("/")
                                   .append(metaData.region)
                                   .append("/")
                                   .append(metaData.service)
                                   .append("/")
                                   .append("request");
    std::string stringToSignV4;
    stringToSignV4.append(metaData.algorithm)
        .append("\n")
        .append(requestTs)
        .append("\n")
        .append(metaData.credentialScope)
        .append("\n")
        .append(hashedCanonReq);
    return stringToSignV4;
}

std::string VolcengineSignV4::hashedCanonicalRequestV4(sdk::AsynRequest& request, MetaData& metaData) {
    unsigned char hashedPlayload[32];
    SHA256((unsigned char*)request.mBody.c_str(), request.mBody.length(), hashedPlayload);
    std::string payloadHash(stringToHex(hashedPlayload, 32));
    request.mHeader["X-Content-Sha256"] = payloadHash;
    request.mHeader["Host"] = request.mHost;

    std::vector<std::pair<std::string, std::string>> signedHeader;
    for (auto iter : request.mHeader) {
        if (strcmp(iter.first.c_str(), "Content-Type") == 0 || strcmp(iter.first.c_str(), "Content-Md5") == 0
            || strcmp(iter.first.c_str(), "Host") == 0 || strcmp(iter.first.c_str(), "X-Security-Token") == 0
            || iter.first.rfind("X-", 0) == 0) {
            signedHeader.emplace_back(toLower(iter.first), iter.second);
        }
    }
    std::sort(signedHeader.begin(), signedHeader.end(), compareByPairKey);

    auto split = "\n";
    std::string canonicalRequest;
    canonicalRequest.append(request.mHTTPMethod)
        .append(split)
        .append(encodePath(request.mUrl))
        .append(split)
        .append(request.mQueryString)
        .append(split);
    std::vector<std::string> keys;
    for (const std::pair<std::string, std::string>& entry : signedHeader) {
        auto key = entry.first;
        keys.emplace_back(key);

        canonicalRequest.append(key);
        canonicalRequest.append(":");
        canonicalRequest.append(entry.second);
        canonicalRequest.append("\n");
    }
    canonicalRequest.append(split);

    auto iter = keys.begin();
    std::string signedHeaders;
    while (iter != keys.end()) {
        if (iter != keys.begin()) {
            signedHeaders.append(";");
        }
        signedHeaders.append(*iter);
        iter++;
    }
    metaData.signedHeaders = signedHeaders;
    canonicalRequest.append(signedHeaders);
    canonicalRequest.append(split);

    if (!payloadHash.empty()) {
        canonicalRequest.append(payloadHash);
    } else {
        canonicalRequest.append(emptySHA256);
    }
    unsigned char hashedRequest[32];
    SHA256((unsigned char*)canonicalRequest.c_str(), canonicalRequest.length(), hashedRequest);
    return stringToHex(hashedRequest, 32);
}

void VolcengineSignV4::prepareRequestV4(sdk::AsynRequest& request) {
    // gen date for sign
    std::time_t now = time(nullptr);
    std::map<std::string, std::string> necessaryDefaults
        = {{"Content-Type", "application/x-www-form-urlencoded; charset=utf-8"},
           {"X-Date", transTimeToFormat(now, iso8601Layout)}};
    for (const auto& iter : necessaryDefaults) {
        auto it = request.mHeader.find(iter.first);
        if (it == request.mHeader.end()) {
            request.mHeader[iter.first] = iter.second;
        }
    }
    if (strcmp(request.mUrl.c_str(), "") == 0) {
        request.mUrl = "/";
    }
}

std::string VolcengineSignV4::UriEncode(const std::string& in, bool encodeSlash) {
    int hexCount = 0;
    std::vector<uint8_t> uint8Char;
    for (int i = 0; i < in.length(); i++) {
        uint8Char.emplace_back((uint8_t)(in[i]));
        if (uint8Char[i] == '/') {
            if (encodeSlash) {
                hexCount++;
            }
        } else if (nonEscape.count(uint8Char[i]) == 0) {
            hexCount++;
        }
    }
    std::vector<char> encoded;
    for (int i = 0; i < in.length(); i++) {
        if (uint8Char[i] == '/') {
            if (encodeSlash) {
                encoded.emplace_back('%');
                encoded.emplace_back('2');
                encoded.emplace_back('F');
            } else {
                encoded.emplace_back(uint8Char[i]);
            }
        } else if (nonEscape.count(uint8Char[i]) == 0) {
            encoded.emplace_back('%');
            encoded.emplace_back("0123456789ABCDEF"[uint8Char[i] >> 4]);
            encoded.emplace_back("0123456789ABCDEF"[uint8Char[i] & 15]);
        } else {
            encoded.emplace_back(uint8Char[i]);
        }
    }
    std::string ret;
    ret.assign(encoded.begin(), encoded.end());
    return ret;
}

std::string VolcengineSignV4::encodePath(const std::string& path) {
    if (path.empty()) {
        return "/";
    }
    return UriEncode(path, false);
}

std::string VolcengineSignV4::EncodeQuery(std::map<std::string, std::string> query) {
    if (query.empty()) {
        return "";
    }
    std::vector<std::pair<std::string, std::string>> signedRes;
    for (const auto& iter : query) {
        signedRes.emplace_back(iter.first, iter.second);
    }
    std::sort(signedRes.begin(), signedRes.end(), compareByPairKey);
    std::string buf;
    auto iter = signedRes.begin();
    while (iter != signedRes.end()) {
        if (iter != signedRes.begin()) {
            buf.append("&");
        }
        auto keyEscaped = UriEncode(iter->first, true);
        buf.append(keyEscaped);
        buf.append("=");
        auto v = UriEncode(iter->second, true);
        buf.append(v);
        iter++;
    }
    return buf;
}
} // namespace logtail
