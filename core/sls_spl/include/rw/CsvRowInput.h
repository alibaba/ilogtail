#pragma once

#include "thirdparty/csv.hpp"
#include "util/Util.h"
#include "IO.h"

namespace apsara::sls::spl {

// TODO: use SIMD while parsing csv lines
class CsvRowInput : public Input {
public:
    CsvRowInput(
            const MemoryPoolPtr& pool,
            const VeloxStringViewVector& rows,
            const csv::CSVFormat& fmt,
            const bool strict,
            const std::vector<std::string>&  header
    ) : pool_(pool), rows_(rows), fmt_(fmt), strict_(strict), header_(header) {}

    ~CsvRowInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t row, std::vector<KV>& pairs, std::string& err);

private:
    const MemoryPoolPtr pool_;
    const VeloxStringViewVector& rows_;
    const csv::CSVFormat fmt_;
    const bool strict_;
    const std::vector<std::string> header_;
    std::vector<BufferPtr> stringBuffers_;
};

}  // namespace apsara::sls::spl