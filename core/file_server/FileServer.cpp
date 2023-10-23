#include "file_server/FileServer.h"

using namespace std;

namespace logtail {

pair<const FileDiscoveryOptions*, const PipelineContext*> FileServer::GetFileDiscoveryOptions(const string& name) const {
    auto itr = mPipelineNameFileDiscoveryOptionsMap.find(name);
    if (itr != mPipelineNameFileDiscoveryOptionsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

pair<const FileReaderOptions*, const PipelineContext*> FileServer::GetFileReaderOptions(const string& name) const {
    auto itr = mPipelineNameFileReaderOptionsMap.find(name);
    if (itr != mPipelineNameFileReaderOptionsMap.end()) {
        return itr->second;
    }
    return make_pair(nullptr, nullptr);
}

void FileServer::AddFileDiscoveryOptions(const string& name, const FileDiscoveryOptions* opts, const PipelineContext* ctx) {
    mPipelineNameFileDiscoveryOptionsMap[name] = make_pair(opts, ctx);
}

void FileServer::AddFileReaderOptions(const string& name, const FileReaderOptions* opts, const PipelineContext* ctx) {
    mPipelineNameFileReaderOptionsMap[name] = make_pair(opts, ctx);
}

} // namespace logtail
