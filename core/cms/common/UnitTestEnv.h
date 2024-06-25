//
// Created by 韩呈杰 on 2024/5/7.
//

#ifndef ARGUSAGENT_UNITTESTENV_H
#define ARGUSAGENT_UNITTESTENV_H

#ifdef ENABLE_COVERAGE
#include <boost/filesystem.hpp>

extern const boost::filesystem::path TEST_CONF_PATH;
extern const boost::filesystem::path TEST_SIC_CONF_PATH;
#endif

#endif //ARGUSAGENT_UNITTESTENV_H
