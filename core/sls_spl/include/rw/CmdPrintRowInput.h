#pragma once

#include "IO.h"

namespace apsara::sls::spl {

// TODO: use SIMD while parsing csv lines
class CmdPrintRowInput : public Input {
public:
    CmdPrintRowInput(std::vector<std::vector<std::string>> fields) : fields_(fields) {}

    ~CmdPrintRowInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getColumn(const int32_t col, std::vector<SplStringPiece>& values, std::string& err) {}


private:
    std::vector<std::vector<std::string>> fields_;
};

}  // namespace apsara::sls::spl