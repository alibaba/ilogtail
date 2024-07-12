#include "provider.h"
#include "config/common_provider/CommonConfigProvider.h"

namespace logtail {

ConfigProvider* GetRemoteConfigProvider() {   
    return CommonConfigProvider::GetInstance();
};

ProfileSender* GetProfileSenderProvider() {
    return ProfileSender::GetInstance();
};

} // namespace logtail