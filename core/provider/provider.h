#pragma once

// #include "app_config/attributes/AttributesProvider.h"
#include "config/provider/ConfigProvider.h"
#include "profile_sender/ProfileSender.h"

namespace logtail {

// AttributesProvider* GetAttributesProvider();

ConfigProvider* GetRemoteConfigProvider();

ProfileSender* GetProfileSenderProvider();    

} // namespace logtail 