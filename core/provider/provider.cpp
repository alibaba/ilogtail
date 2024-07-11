#include "provider.h"
#include "config/provider/CommonConfigProvider.h"

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