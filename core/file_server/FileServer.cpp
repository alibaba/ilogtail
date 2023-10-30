#include "file_server/FileServer.h"

using namespace std;

namespace logtail {

FileDiscoveryConfig FileServer::GetFileDiscoveryConfig(const string& name) const {
    auto itr = mPipelineNameFileDiscoveryConfigsMap.find(name);
    if (itr != mPipelineNameFileDiscoveryConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

FileReaderConfig FileServer::GetFileReaderConfig(const string& name) const {
    auto itr = mPipelineNameFileReaderConfigsMap.find(name);
    if (itr != mPipelineNameFileReaderConfigsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

void FileServer::AddFileDiscoveryConfig(const string& name,
                                         const FileDiscoveryOptions* opts,
                                         const PipelineContext* ctx) {
    mPipelineNameFileDiscoveryConfigsMap[name] = make_pair(opts, ctx);
}

void FileServer::AddFileReaderConfig(const string& name, const FileReaderOptions* opts, const PipelineContext* ctx) {
    mPipelineNameFileReaderConfigsMap[name] = make_pair(opts, ctx);
}

} // namespace logtail
