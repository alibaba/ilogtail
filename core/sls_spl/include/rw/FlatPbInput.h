#pragma once

#include <memory>
#include "IO.h"
#include "SlsLogPbParser.h"
#include "SplStringPiece.h"

namespace apsara::sls::spl {

class FlatPbInput : public Input {
   private:
    SlsLogGroupFlatPb slsLogGroupFlatPb_;

   public:
    FlatPbInput(SlsLogGroupFlatPb& slsLogGroupFlatPb)
        : slsLogGroupFlatPb_(slsLogGroupFlatPb) {}
    ~FlatPbInput() {}
    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t row, std::vector<KV>& pairs, std::string& err);
    virtual void getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err) {}
};

using SlsLogGroupInputPtr = std::shared_ptr<FlatPbInput>;

}  // namespace apsara::sls::spl