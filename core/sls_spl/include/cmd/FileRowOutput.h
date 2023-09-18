#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "rw/IO.h"

namespace apsara::sls::spl {

class FileRowOutput : public Output {
   private:
    std::string taskLabel_;
    int32_t rowSize_;
    int32_t currentRow_;
    std::string fileName_;
    std::ofstream ofs_;
    std::vector<SplStringPiece> columnNames_;
    std::unordered_map<int32_t, SplStringPiece> constColumns_;
    bool isMapFormat_;

   public:
    FileRowOutput(const std::string& taskLabel = "", const std::string fileName = "", const bool isMapFormat = true)
        : taskLabel_(taskLabel), fileName_(fileName), isMapFormat_(isMapFormat) {}
    virtual ~FileRowOutput() {}
    virtual void setHeader(const IOHeader& header, std::string& err);
    virtual void addRow(
        const std::vector<SplStringPiece>& row,
        const ErrorKV& errorKV,
        std::string& error);
    virtual void finish(std::string& error);
};

using FileRowOutputPtr = std::shared_ptr<FileRowOutput>;

}  // namespace apsara::sls::spl