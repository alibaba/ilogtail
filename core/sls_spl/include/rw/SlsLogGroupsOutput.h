#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "IO.h"
#include "SlsLogPbWriter.h"


namespace apsara::sls::spl {

class SlsLogGroupsOutput : public Output {
public:
    SlsLogGroupsOutput(MemoryArena* memoryArena) : memoryArena_(memoryArena) {}
    virtual ~SlsLogGroupsOutput() {}
    virtual void setHeader(const IOHeader& header, std::string& err);
    virtual void addRow(
        const std::vector<SplStringPiece>& row, 
        const ErrorKV& errorKV, 
        std::string& error
    );
    virtual void finish(std::string& error) {};

    int32_t bufferSize();
    const std::vector<SlsLogGroupWriter*> logGroupWriters();

private:
    MemoryArena* memoryArena_;
    std::vector<std::string> fieldNames_;
    int32_t idxTime_ = -1;
    int32_t idxSource_ = -1;
    int32_t idxTopic_ = -1;
    std::vector<int32_t> idxTags_;
    std::vector<int32_t> idxContents_;
    std::vector<SlsLogGroupWriter*> logGroupWriters_;
    std::unordered_map<std::string, int32_t> logGroupMapping_;
    uint32_t defaultTime_;
    bool flatMeta_;

    void putMeta(uint32_t key, const SplStringPiece& val, std::shared_ptr<SlsLogWriter>& metas);
    uint32_t getLogGroup(const std::vector<SplStringPiece>& row, uint32_t& time);
};

}  // namespace apsara::sls::spl
