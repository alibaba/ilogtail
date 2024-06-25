#ifndef _CLOUD_SIGNATRUE_H_
#define _CLOUD_SIGNATRUE_H_

#include <string>

namespace cloudMonitor {
#include "common/test_support"
    class CloudSignature {
    public:
        static void Calculate(const std::string &content, const std::string &secret, std::string &result);

        static void CalculateMetric(const std::string &content, const std::string &secret, std::string &result);

    private:
        static int AesEncrypted(const std::string &src, const std::string &key, std::string &result);
    };
#include "common/test_support"
}
#endif
