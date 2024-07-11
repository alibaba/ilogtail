#pragma once

#include "config/provider/ConfigProvider.h"
#include "profile_sender/ProfileSender.h"

namespace logtail {


ConfigProvider* GetRemoteConfigProvider();


ProfileSender* GetProfileSenderProvider();    
} 