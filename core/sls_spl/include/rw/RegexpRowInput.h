#pragma once

#include "IO.h"
#include "util/Regexp.h"
#include "util/Type.h"

namespace apsara::sls::spl {

class RegexpRowInput : public Input {
   public:
    RegexpRowInput(
        const Re2StringPieceVector& rows,
        NamedExtractor* extractor)
        : rows_(rows), extractor_(extractor) {}

    ~RegexpRowInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t row, std::vector<KV>& pairs, std::string& err);

   private:
    const Re2StringPieceVector rows_;
    NamedExtractor* extractor_;
};

}  // namespace apsara::sls::spl