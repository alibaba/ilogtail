#include "logger/Logger.h"
#include "sls_spl/LogtailLogger.h"

namespace apsara::sls::spl {

void LogtailLogger::log(const LOG_LEVEL& level, const std::string& file, const int32_t line, const std::string& content) {
    //LOG(INFO) << "level: " << level << ", file: " << file << ", line: " << line << ", content: " << content;
    LOG_INFO(sLogger, ("level", level)("file", file)("line", line)("content", content));
}

}  // namespace apsara::sls::spl