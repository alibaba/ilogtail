#pragma once

#include <memory>
#include "IO.h"
#include "SlsLogPbParser.h"

namespace apsara::sls::spl {

class SlsLogGroupInput : public Input {
   private:
    SlsLogGroupFlatPb* slsLogGroupFlatPb_;
    std::vector<std::string> tags_;

   public:
    SlsLogGroupInput(SlsLogGroupFlatPb* slsLogGroupFlatPb)
        : slsLogGroupFlatPb_(slsLogGroupFlatPb) {}
    ~SlsLogGroupInput() {}
    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t row, std::vector<KV>& pairs, std::string& err);
    virtual void getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err) {}
};

using SlsLogGroupInputPtr = std::shared_ptr<SlsLogGroupInput>;

}  // namespace apsara::sls::spl
