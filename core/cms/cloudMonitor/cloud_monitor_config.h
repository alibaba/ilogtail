#ifndef _CLOUD_MONITOR_CONFIG_H_
#define _CLOUD_MONITOR_CONFIG_H_

#include <string>

class Config;
#ifdef ENABLE_COVERAGE
Config *newCmsConfig(std::string path = {});
#endif

#endif