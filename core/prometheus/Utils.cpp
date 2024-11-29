#include "prometheus/Utils.h"

#include <xxhash/xxhash.h>

#include <iomanip>

#include "common/StringTools.h"
#include "models/StringView.h"

using namespace std;

namespace logtail {
string URLEncode(const string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (char c : value) {
        // Keep alphanumeric characters and other safe characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        // Any other characters are percent-encoded
        escaped << '%' << setw(2) << int((unsigned char)c);
    }

    return escaped.str();
}

std::string SecondToDuration(uint64_t duration) {
    return (duration % 60 == 0) ? ToString(duration / 60) + "m" : ToString(duration) + "s";
}

uint64_t DurationToSecond(const std::string& duration) {
    // check duration format <duration>s or <duration>m
    if (duration.size() <= 1 || !IsNumber(duration.substr(0, duration.size() - 1))) {
        return 0;
    }
    if (EndWith(duration, "s")) {
        return stoll(duration.substr(0, duration.find('s')));
    }
    if (EndWith(duration, "m")) {
        return stoll(duration.substr(0, duration.find('m'))) * 60;
    }
    return 0;
}

// <size>: a size in bytes, e.g. 512MB. A unit is required. Supported units: B, KB, MB, GB, TB, PB, EB.
uint64_t SizeToByte(const std::string& size) {
    auto inputSize = size;
    uint64_t res = 0;
    if (size.empty()) {
        res = 0;
    } else if (EndWith(inputSize, "KiB") || EndWith(inputSize, "K") || EndWith(inputSize, "KB")) {
        if (!IsNumber(inputSize.substr(0, inputSize.find('K')))) {
            return 0;
        }
        inputSize = inputSize.substr(0, inputSize.find('K'));
        res = stoll(inputSize) * 1024;
    } else if (EndWith(inputSize, "MiB") || EndWith(inputSize, "M") || EndWith(inputSize, "MB")) {
        if (!IsNumber(inputSize.substr(0, inputSize.find('M')))) {
            return 0;
        }
        inputSize = inputSize.substr(0, inputSize.find('M'));
        res = stoll(inputSize) * 1024 * 1024;
    } else if (EndWith(inputSize, "GiB") || EndWith(inputSize, "G") || EndWith(inputSize, "GB")) {
        if (!IsNumber(inputSize.substr(0, inputSize.find('G')))) {
            return 0;
        }
        inputSize = inputSize.substr(0, inputSize.find('G'));
        res = stoll(inputSize) * 1024 * 1024 * 1024;
    } else if (EndWith(inputSize, "TiB") || EndWith(inputSize, "T") || EndWith(inputSize, "TB")) {
        if (!IsNumber(inputSize.substr(0, inputSize.find('T')))) {
            return 0;
        }
        inputSize = inputSize.substr(0, inputSize.find('T'));
        res = stoll(inputSize) * 1024 * 1024 * 1024 * 1024;
    } else if (EndWith(inputSize, "PiB") || EndWith(inputSize, "P") || EndWith(inputSize, "PB")) {
        if (!IsNumber(inputSize.substr(0, inputSize.find('P')))) {
            return 0;
        }
        inputSize = inputSize.substr(0, inputSize.find('P'));
        res = stoll(inputSize) * 1024 * 1024 * 1024 * 1024 * 1024;
    } else if (EndWith(inputSize, "EiB") || EndWith(inputSize, "E") || EndWith(inputSize, "EB")) {
        if (!IsNumber(inputSize.substr(0, inputSize.find('E')))) {
            return 0;
        }
        inputSize = inputSize.substr(0, inputSize.find('E'));
        res = stoll(inputSize) * 1024 * 1024 * 1024 * 1024 * 1024 * 1024;
    } else if (EndWith(inputSize, "B")) {
        if (!IsNumber(inputSize.substr(0, inputSize.find('B')))) {
            return 0;
        }
        inputSize = inputSize.substr(0, inputSize.find('B'));
        res = stoll(inputSize);
    }
    return res;
}

bool IsValidMetric(const StringView& line) {
    for (auto c : line) {
        if (c == ' ' || c == '\t') {
            continue;
        }
        if (c == '#') {
            return false;
        }
        return true;
    }
    return false;
}

void SplitStringView(const std::string& s, char delimiter, std::vector<StringView>& result) {
    size_t start = 0;
    size_t end = 0;

    while ((end = s.find(delimiter, start)) != std::string::npos) {
        result.emplace_back(s.data() + start, end - start);
        start = end + 1;
    }
    if (start < s.size()) {
        result.emplace_back(s.data() + start, s.size() - start);
    }
}

bool IsNumber(const std::string& str) {
    return !str.empty() && str.find_first_not_of("0123456789") == std::string::npos;
}

uint64_t GetRandSleepMilliSec(const std::string& key, uint64_t intervalSeconds, uint64_t currentMilliSeconds) {
    // Pre-compute the inverse of the maximum value of uint64_t
    static constexpr double sInverseMaxUint64 = 1.0 / static_cast<double>(std::numeric_limits<uint64_t>::max());

    uint64_t h = XXH64(key.c_str(), key.length(), 0);

    // Normalize the hash to the range [0, 1]
    double normalizedH = static_cast<double>(h) * sInverseMaxUint64;

    // Scale the normalized hash to milliseconds
    auto randSleep = static_cast<uint64_t>(std::ceil(intervalSeconds * 1000.0 * normalizedH));

    // calculate sleep window start offset, apply random sleep
    uint64_t sleepOffset = currentMilliSeconds % (intervalSeconds * 1000ULL);
    if (randSleep < sleepOffset) {
        randSleep += intervalSeconds * 1000ULL;
    }
    randSleep -= sleepOffset;
    return randSleep;
}

namespace prom {

    std::string CurlCodeToString(uint64_t code) {
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
} // namespace prom
} // namespace logtail
