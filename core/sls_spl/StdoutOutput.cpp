#include "StdoutOutput.h"
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace apsara::sls::spl {

void StdoutOutput::setHeader(const IOHeader& header, std::string& err) {
    currentRow_ = 0;
    rowSize_ = header.rowSize;
    columnNames_ = header.columnNames;
    for (auto& constCol : header.constCols) {
        constColumns_.emplace(constCol.first, constCol.second.ToString());
    }
    std::cout << "====================================================" << std::endl;
}

void StdoutOutput::addRow(
    const std::vector<SplStringPiece>& row,
    const ErrorKV& errorKV,
    std::string& error) {
    std::stringstream buf;
    for (auto col = 0; col < row.size(); ++col) {
        if (row[col].hasValue()) {
            buf << std::setw(36) << (columnNames_[col]) << ": " << row[col].ToString() << std::endl;
        } else if (constColumns_.count(col)) {
            buf << std::setw(36) << (columnNames_[col]) << ": " << constColumns_[col] << std::endl;
        } else {
            buf << std::setw(36) << (columnNames_[col]) << ": " << std::endl;
        }
    }
    if (!errorKV.second.empty()) {
        buf << std::setw(36) << ("__error__") << ": " << errorKV.second << std::endl;
    }
    if (currentRow_ == rowSize_ - 1) {
        std::cout << buf.str();
    } else {
        std::cout << buf.str() << std::endl;
    }
    ++currentRow_;
    if (withSleep_) {
        sleep(1);
    }
}

void StdoutOutput::finish(std::string& error) {
    std::cout << "#######====================================================#######" << std::endl;
    std::cout << "Results: " << rowSize_ << " Rows.\n"
              << std::endl;
}

}  // namespace apsara::sls::spl