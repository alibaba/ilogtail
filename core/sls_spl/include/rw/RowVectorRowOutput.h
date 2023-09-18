#pragma once

#include <velox/vector/ComplexVector.h>
#include <string>
#include <vector>
#include "IO.h"
#include "vector/VectorBuilder.h"

namespace apsara::sls::spl {

class RowVectorRowOutput : public Output {
   private:
    std::string taskLabel_;
    int32_t rowSize_;
    int32_t currentRow_;
    std::vector<SplStringPiece> columnNames_;
    std::unordered_map<int32_t, std::string> constColumns_;
    VectorBuilderPtr vbPtr_;
    facebook::velox::RowVectorPtr rowVectorPtr_;

   public:
    RowVectorRowOutput(const std::string& taskLabel, const VectorBuilderPtr& vbPtr)
        : taskLabel_(taskLabel), vbPtr_(vbPtr) {}
    virtual ~RowVectorRowOutput() {
    }
    virtual void setHeader(const IOHeader& header, std::string& err);
    virtual void addRow(
        const std::vector<SplStringPiece>& row,
        const ErrorKV& errorKV,
        std::string& error);
    virtual void finish(std::string& error);
    facebook::velox::RowVectorPtr getOutput() {
        return rowVectorPtr_;
    }
};

using RowVectorRowOutputPtr = std::shared_ptr<RowVectorRowOutput>;

}  // namespace apsara::sls::spl