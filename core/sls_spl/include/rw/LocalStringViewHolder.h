#pragma once

#include <velox/buffer/Buffer.h>
#include <velox/type/StringView.h>
#include "SplStringPiece.h"
#include "util/Type.h"

namespace apsara::sls::spl {

// used for inline stringview
class LocalStringViewHolder {
   private:
    MemoryPoolPtr pool_;
    size_t totalSize_{0};
    size_t currentSize_{0};
    size_t currentOffset_{0};
    std::vector<facebook::velox::BufferPtr> bufferPtrs_;

   public:
    LocalStringViewHolder(const MemoryPoolPtr pool, const size_t initSize = 512)
        : pool_(pool), totalSize_(initSize) {}
    ~LocalStringViewHolder() {}
    void reset();
    void fromStringView(const facebook::velox::StringView& sv, SplStringPiece& spb);
};

}  // namespace apsara::sls::spl