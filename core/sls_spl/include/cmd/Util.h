#pragma once

#include <velox/common/time/Timer.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <presto_cpp/external/json/json.hpp>
#include <string>
#include <vector>
#include "FileRowOutput.h"
#include "logger/StdoutLogger.h"
#include "pipeline/Error.h"
#include "pipeline/PipelineFactory.h"
#include "rw/RowVectorInput.h"
#include "rw/SlsLogPbWriter.h"
#include "util/Util.h"

using namespace nlohmann;
using namespace apsara::sls::spl;

std::string formatSize(long long size);

int64_t getFileSize(const std::string& filename);

std::string formatTime(long long size);

std::string formatNumber(int num);

std::string formatPercentage(int32_t numerator, int32_t denominator, int precision);

void loadMapVectorFromFile(
    const std::string& fileName,
    int64_t& fileSize,
    int32_t& rowCount,
    std::vector<std::string>& columnNames,
    std::vector<std::unordered_map<std::string, std::string>>& rows);

RowVectorPtr loadFieldRVFromFile(
    const VectorBuilderPtr& vbPtr,
    const std::string& fileName,
    int64_t& fileSize,
    int32_t& rowCount);

std::shared_ptr<SlsLogGroupFlatPb> loadLogGroupFromFile(
    SlsLogGroupWriter& writer,
    const std::string& fileName,
    int64_t& fileSize,
    int32_t& rowCount);

class NullRowOutput : public Output {
   private:
    std::string taskLabel_;

   public:
    NullRowOutput(const std::string& taskLabel = "")
        : taskLabel_(taskLabel) {}
    virtual ~NullRowOutput() {}
    virtual void addRow(const std::vector<std::optional<SplStringPiece>>& row, const ErrorKV& errorKV, std::string& error) {}
    virtual void finish(std::string& error) {}
};

class MapRowOutput : public Output {
   public:
    MapRowOutput() {}

    ~MapRowOutput() {}

    virtual void setHeader(IOHeader& header, std::string& err) {
        rowSize_ = header.rowSize;
        for (auto& cn : header.columnNames) {
            columnNames_.emplace_back(cn.ToString());
        }
        rows_.reserve(rowSize_);
    }
    virtual void getRow(const int32_t rowIndex, std::vector<KV>& pairs, std::string& err) {
        auto& currentRow = rows_[rowIndex];
        for (auto& kv : currentRow) {
            pairs.emplace_back(kv.first, kv.second);
        }
    }

    virtual void addRow(const std::vector<SplStringPiece>& row, const ErrorKV& errorKV, std::string& err) {
        std::unordered_map<std::string, std::string> rowMap;
        for (auto i = 0; i < columnNames_.size(); ++i) {
            if (!row[i].empty()) {
                rowMap.emplace(columnNames_[i], row[i].ToString());
            } else {
                rowMap.emplace(columnNames_[i], "");
            }
        }
        rows_.emplace_back(rowMap);
    }

   private:
    std::vector<std::unordered_map<std::string, std::string>> rows_;
    int32_t rowSize_;
    std::vector<std::string> columnNames_;
};