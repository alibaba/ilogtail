#pragma once

#include <string>
#include <vector>
#include "IO.h"

namespace apsara::sls::spl {

class StdoutRowOutput : public Output {
   private:
    std::string taskLabel_;
    int32_t rowSize_;
    int32_t currentRow_;
    std::vector<SplStringPiece> columnNames_;
    std::unordered_map<int32_t, std::string> constColumns_;
    bool withSleep_;

   public:
    StdoutRowOutput(const std::string& taskLabel = "", bool withSleep = false)
        : taskLabel_(taskLabel), withSleep_(withSleep) {}
    virtual ~StdoutRowOutput() {
        
    }
    virtual void setHeader(const IOHeader& header, std::string& err);
    virtual void addRow(
        const std::vector<SplStringPiece>& row, 
        const ErrorKV& errorKV, 
        std::string& error);
    virtual void finish(std::string& error);
};

using StdoutRowOutputPtr = std::shared_ptr<StdoutRowOutput>;

}  // namespace apsara::sls::spl