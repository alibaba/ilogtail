#include "provider.h"
#include "config/common_provider/CommonConfigProvider.h"

namespace logtail {

// AttributesProvider* GetAttributesProvider() {
//     static AttributesProvider instance;
//     return &instance;
// };

ConfigProvider* GetRemoteConfigProvider() {   
    return CommonConfigProvider::GetInstance();
};

ProfileSender* GetProfileSenderProvider() {
    return ProfileSender::GetInstance();
};

} // namespace logtail