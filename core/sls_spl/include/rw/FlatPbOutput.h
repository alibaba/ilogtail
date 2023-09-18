#pragma once

#include <string>
#include <vector>
#include "spl/rw/IO.h"
#include "spl/rw/SlsLogPbWriter.h"
#include "spl/rw/sls_allocator.h"

namespace apsara::sls::spl {

class FlatPbOutput : public Output {
   private:
    MemoryArena* memoryArena_;
    std::string taskLabel_;
    int32_t currentRow_;
    IOHeader* ioHeader_;
    SlsLogGroupWriter* outLogGroupWriter_ = nullptr;

   public:
    FlatPbOutput(MemoryArena* memoryArena, const std::string& taskLabel = "")
        : memoryArena_(memoryArena), taskLabel_(taskLabel) {
        outLogGroupWriter_ = new SlsLogGroupWriter(memoryArena_);
    }
    virtual ~FlatPbOutput() {
        delete outLogGroupWriter_;
    }
    virtual void setHeader(const IOHeader& header, std::string& err);
    virtual void addRow(
        const std::vector<SplStringPiece>& row,
        const ErrorKV& errorKV,
        std::string& error);
    virtual void finish(std::string& error) {}
};

using FlatPbOutputPtr = std::shared_ptr<FlatPbOutput>;

}  // namespace apsara::sls::spl