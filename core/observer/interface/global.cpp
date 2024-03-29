// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
// Created by evan on 2022/10/19.
//

#include "global.h"

namespace logtail {
namespace observer {
    std::string kLocalInfo = "local_info";
    std::string kRemoteInfo = "remote_info";
    std::string kRole = "role";
    std::string kType = "type";
    std::string kConnId = "conn_id";
    std::string kConnType = "conn_type";
    std::string kLocalAddr = "_local_addr_";
    std::string kLocalPort = "_local_port_";
    std::string kRemoteAddr = "_remote_addr_";
    std::string kRemotePort = "_remote_port_";
    std::string kInterval = "interval";
    std::string kSendBytes = "send_bytes";
    std::string kRecvBytes = "recv_bytes";
    std::string kSendpackets = "send_packets";
    std::string kRecvPackets = "recv_packets";
    std::string kQueryCmd = "query_cmd";
    std::string kQuery = "query";
    std::string kStatus = "status";
    std::string kReqType = "req_type";
    std::string kReqDomain = "req_domain";
    std::string kReqResource = "req_resource";
    std::string kRespStatus = "resp_status";
    std::string kRespCode = "resp_code";
    std::string kLatencyNs = "latency_ns";
    std::string kReqBytes = "req_bytes";
    std::string kRespBytes = "resp_bytes";
    std::string kCount = "count";
    std::string kProtocol = "protocol";
    std::string kVersion = "version";
    std::string kTdigestLatency = "tdigest_latency";

} // namespace observer


} // namespace logtail