#pragma once

#include "IO.h"
#include "util/Regexp.h"
#include "util/Type.h"

namespace apsara::sls::spl {

class KvRowInput : public Input {
   public:
    KvRowInput(
        const MemoryPoolPtr& pool,
        const Re2StringPieceVector& rows,
        KvExtractor* extractor,
        const std::string& prefix)
        : pool_(pool), rows_(rows), extractor_(extractor), prefix_(prefix) {}

    ~KvRowInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t row, std::vector<KV>& pairs, std::string& err);

   private:
    const MemoryPoolPtr pool_;
    const Re2StringPieceVector rows_;
    KvExtractor* extractor_;
    const std::string prefix_;
    std::vector<BufferPtr> stringBuffers_;
};

}  // namespace apsara::sls::spl