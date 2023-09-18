#pragma once

#include <velox/vector/ComplexVector.h>
#include "IO.h"
#include "SlsLogPbParser.h"
#include "rw/LocalStringViewHolder.h"
#include "util/Type.h"

namespace apsara::sls::spl {

// for test
class RowVectorInput : public Input {
   private:
    MemoryPoolPtr pool_;
    facebook::velox::RowVectorPtr rowVectorPtr_;
    LocalStringViewHolder localStringViewHolder_;

   public:
    RowVectorInput(
        const MemoryPoolPtr& pool,
        const facebook::velox::RowVectorPtr& rowVectorPtr)
        : pool_(pool), rowVectorPtr_(rowVectorPtr), localStringViewHolder_(pool) {
        localStringViewHolder_.reset();
    }

    ~RowVectorInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);

    // get non-const columns
    virtual void getColumn(
        const int32_t colIndex,
        std::vector<SplStringPiece>& values,
        std::string& err);

    virtual bool isColumnar() {
        return true;
    }
};

using RowVectorRowInputPtr = std::shared_ptr<RowVectorInput>;

}  // namespace apsara::sls::spl