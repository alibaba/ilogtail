#include "prometheus/PromSelfMonitor.h"

#include <map>
#include <string>
#include <unordered_map>

#include "monitor/MetricTypes.h"
#include "monitor/metric_constants/MetricConstants.h"
using namespace std;

namespace logtail {

void PromSelfMonitorUnsafe::InitMetricManager(const std::unordered_map<std::string, MetricType>& metricKeys,
                                        const MetricLabels& labels) {
    auto metricLabels = std::make_shared<MetricLabels>(labels);
    mPluginMetricManagerPtr = std::make_shared<PluginMetricManager>(metricLabels, metricKeys, MetricCategory::METRIC_CATEGORY_PLUGIN_SOURCE);
}

void PromSelfMonitorUnsafe::AddCounter(const std::string& metricName, uint64_t statusCode, uint64_t val) {
    auto& status = StatusToString(statusCode);
    if (!mMetricsCounterMap.count(metricName) || !mMetricsCounterMap[metricName].count(status)) {
        mMetricsCounterMap[metricName][status] = GetOrCreateReentrantMetricsRecordRef(status)->GetCounter(metricName);
    }
    mMetricsCounterMap[metricName][status]->Add(val);
}

void PromSelfMonitorUnsafe::SetIntGauge(const std::string& metricName, uint64_t statusCode, uint64_t value) {
    auto& status = StatusToString(statusCode);
    if (!mMetricsIntGaugeMap.count(metricName) || !mMetricsIntGaugeMap[metricName].count(status)) {
        mMetricsIntGaugeMap[metricName][status] = GetOrCreateReentrantMetricsRecordRef(status)->GetIntGauge(metricName);
    }
    mMetricsIntGaugeMap[metricName][status]->Set(value);
}

ReentrantMetricsRecordRef PromSelfMonitorUnsafe::GetOrCreateReentrantMetricsRecordRef(const std::string& status) {
    if (mPluginMetricManagerPtr == nullptr) {
        return nullptr;
    }
    if (!mPromStatusMap.count(status)) {
        mPromStatusMap[status]
            = mPluginMetricManagerPtr->GetOrCreateReentrantMetricsRecordRef({{METRIC_LABEL_KEY_STATUS, status}});
    }
    return mPromStatusMap[status];
}

std::string& PromSelfMonitorUnsafe::StatusToString(uint64_t status) {
    static string sHttp0XX = "0XX";
    static string sHttp1XX = "1XX";
    static string sHttp2XX = "2XX";
    static string sHttp3XX = "3XX";
    static string sHttp4XX = "4XX";
    static string sHttp5XX = "5XX";
    static string sHttpOther = "other";
    if (status < 100) {
        return sHttp0XX;
    } else if (status < 200) {
        return sHttp1XX;
    } else if (status < 300) {
        return sHttp2XX;
    } else if (status < 400) {
        return sHttp3XX;
    } else if (status < 500) {
        return sHttp4XX;
    } else if (status < 500) {
        return sHttp5XX;
    } else {
        return sHttpOther;
    }
}

std::string PromSelfMonitorUnsafe::CurlCodeToString(uint64_t code) {
    static map<uint64_t, string> sCurlCodeMap = {{0, "OK"},
                                                 {7, "ERR_CONN_REFUSED"},
                                                 {9, "ERR_ACCESS_DENIED"},
                                                 {28, "ERR_TIMEOUT"},
                                                 {35, "ERR_SSL_CONN_ERR"},
                                                 {51, "ERR_SSL_CERT_ERR"},
                                                 {52, "ERR_SERVER_RESPONSE_NONE"},
                                                 {55, "ERR_SEND_DATA_FAILED"},
                                                 {56, "ERR_RECV_DATA_FAILED"}};
    static string sCurlOther = "ERR_UNKNOWN";
    if (sCurlCodeMap.count(code)) {
        return sCurlCodeMap[code];
    }
    return sCurlOther;
}

// inused curl error code:
// 7 Couldn't connect to server
// 9 Access denied to remote resource
// 28 Timeout was reached
// 35 SSL connect error
// 51 SSL peer certificate or SSH remote key was not OK
// 52 Server returned nothing (no headers, no data)
// 55 Failed sending data to the peer
// 56 Failure when receiving data from the peer

// unused
// 0 No error
// 1 Unsupported protocol
// 2 Failed initialization
// 3 URL using bad/illegal format or missing URL
// 4 A requested feature, protocol or option was not found built-in in this libcurl due to a build-time decision.
// 5 Couldn't resolve proxy name
// 6 Couldn't resolve host name
// 8 Weird server reply
// 10 FTP: The server failed to connect to data port
// 11 FTP: unknown PASS reply
// 12 FTP: Accepting server connect has timed out
// 13 FTP: unknown PASV reply
// 14 FTP: unknown 227 response format
// 15 FTP: can't figure out the host in the PASV response
// 16 Error in the HTTP2 framing layer
// 17 FTP: couldn't set file type
// 18 Transferred a partial file
// 19 FTP: couldn't retrieve (RETR failed) the specified file
// 20 Unknown error
// 21 Quote command returned error
// 22 HTTP response code said error
// 23 Failed writing received data to disk/application
// 24 Unknown error
// 25 Upload failed (at start/before it took off)
// 26 Failed to open/read local data from file/application
// 27 Out of memory
// 29 Unknown error
// 30 FTP: command PORT failed
// 31 FTP: command REST failed
// 32 Unknown error
// 33 Requested range was not delivered by the server
// 34 Internal problem setting up the POST
// 36 Couldn't resume download
// 37 Couldn't read a file:// file
// 38 LDAP: cannot bind
// 39 LDAP: search failed
// 40 Unknown error
// 41 A required function in the library was not found
// 42 Operation was aborted by an application callback
// 43 A libcurl function was given a bad argument
// 44 Unknown error
// 45 Failed binding local connection end
// 46 Unknown error
// 47 Number of redirects hit maximum amount
// 48 An unknown option was passed in to libcurl
// 49 Malformed telnet option
// 50 Unknown error
// 53 SSL crypto engine not found
// 54 Can not set SSL crypto engine as default
// 57 Unknown error
// 58 Problem with the local SSL certificate
// 59 Couldn't use specified SSL cipher
// 60 Peer certificate cannot be authenticated with given CA certificates
// 61 Unrecognized or bad HTTP Content or Transfer-Encoding
// 62 Invalid LDAP URL
// 63 Maximum file size exceeded
// 64 Requested SSL level failed
// 65 Send failed since rewinding of the data stream failed
// 66 Failed to initialise SSL crypto engine
// 67 Login denied
// 68 TFTP: File Not Found
// 69 TFTP: Access Violation
// 70 Disk full or allocation exceeded
// 71 TFTP: Illegal operation
// 72 TFTP: Unknown transfer ID
// 73 Remote file already exists
// 74 TFTP: No such user
// 75 Conversion failed
// 76 Caller must register CURLOPT_CONV_ callback options
// 77 Problem with the SSL CA cert (path? access rights?)
// 78 Remote file not found
// 79 Error in the SSH layer
// 80 Failed to shut down the SSL connection
// 81 Socket not ready for send/recv
// 82 Failed to load CRL file (path? access rights?, format?)
// 83 Issuer check against peer certificate failed
// 84 FTP: The server did not accept the PRET command.
// 85 RTSP CSeq mismatch or invalid CSeq
// 86 RTSP session error
// 87 Unable to parse FTP file list
// 88 Chunk callback failed
// 89 The max connection limit is reached
// 90 SSL public key does not match pinned public key
// 91 SSL server certificate status verification FAILED
// 92 Stream error in the HTTP/2 framing layer
// 93 API function called from within callback
// 94 Unknown error

} // namespace logtail