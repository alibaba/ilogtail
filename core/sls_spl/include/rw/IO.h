#pragma once

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "SplStringPiece.h"

namespace apsara::sls::spl {

using ErrorKV = std::pair<int32_t, SplStringPiece>;
using KV = std::pair<SplStringPiece, SplStringPiece>;

struct IOHeader {
    int32_t rowSize;
    std::vector<SplStringPiece> columnNames;
    std::unordered_map<int32_t, SplStringPiece> constCols;
};

class Input {
   public:
    Input() {}
    virtual ~Input(){};
    virtual void getHeader(IOHeader& header, std::string& err) {
        header.rowSize = 0;
    }

    // here err doesn't break pipeline, pipeline will skip this row
    virtual void getRow(
        const int32_t row,
        std::vector<KV>& pairs,
        std::string& err) {}

    // get non-const columns
    virtual void getColumn(
        const int32_t colIndex,
        std::vector<SplStringPiece>& values,
        std::string& err) {}

    virtual bool isColumnar() {
        return false;
    }
};

using InputPtr = std::shared_ptr<Input>;

class Output {
   public:
    Output() {}
    virtual ~Output() {}

    void setSettings(std::unordered_map<std::string, std::string> settings) {
        settings_ = settings;
    }

    virtual void setHeader(
        const IOHeader& header,
        std::string& err) {}

    virtual void addRow(
        const std::vector<SplStringPiece>& row,
        const ErrorKV& errorKV,
        std::string& err) {}

    // get non-const columns
    virtual void addColumn(
        const int32_t colIndex,
        const std::vector<SplStringPiece>& columnVals,
        std::string& err) {}

    // for both row/column
    virtual void finish(std::string& err) {}

    virtual bool isColumnar() {
        return false;
    }

   protected:
    std::unordered_map<std::string, std::string> settings_;
};

using OutputPtr = std::shared_ptr<Output>;

}  // namespace apsara::sls::spl