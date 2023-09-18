#pragma once

#include "util/Json.h"
#include "util/Util.h"
#include "IO.h"

namespace apsara::sls::spl {

class JsonRowInput : public Input {
public:
    JsonRowInput(
        const MemoryPoolPtr& pool,
        const JsonVector& rows, 
        const std::string& prefix
    ) : pool_(pool), rows_(rows), prefix_(prefix) {}

    ~JsonRowInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t row, std::vector<KV>& pairs, std::string& err);

private:
    const MemoryPoolPtr pool_;
    const JsonVector rows_;
    const std::string prefix_;
    std::vector<BufferPtr> stringBuffers_;
};

}  // namespace apsara::sls::spl