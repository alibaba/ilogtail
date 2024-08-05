#include "prometheus/Utils.h"

#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>

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
} // namespace logtail
