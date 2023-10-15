#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include "table/TableIterator.h"

namespace logtail {
class Table {
    friend bool operator==(const Table&, const Table&);
    friend bool operator!=(const Table&, const Table&);

public:
    virtual ~Table() = default;
    Table(Table &&rhs) = default;
    Table& operator=(Table &&rhs) = default;

    static std::unique_ptr<Table> CreateTable(const std::string& extension);

    virtual bool parse(const std::string& content) = 0;
    virtual std::string serialize() const = 0;

    virtual bool is_null() const = 0;
    virtual bool is_bool() const = 0;
    virtual bool is_int() const = 0;
    virtual bool is_uint() const = 0;
    virtual bool is_string() const = 0;
    virtual bool is_array() const = 0;
    virtual bool is_object() const = 0;

    virtual bool as_bool() const = 0;
    virtual int32_t as_int() const = 0;
    virtual uint32_t as_uint() const = 0;
    virtual std::string as_string() const = 0;

    virtual operator bool() const = 0;

    // for both array and object
    virtual std::size_t size() const = 0;
    virtual bool empty() const = 0;
    virtual void clear() = 0;
    virtual TableIterator erase(TableIterator pos) = 0;
    virtual TableIterator begin() noexcept = 0;
    virtual TableIterator end() noexcept = 0;

    // for array only
    virtual Table& operator[](std::size_t index) = 0;
    virtual TableIterator insert(TableIterator pos, const Table& table) = 0;
    virtual TableIterator insert(TableIterator pos, Table&& table) = 0;
    virtual void push_back(const Table& table) = 0;
    virtual void push_back(Table&& table) = 0;
    virtual void pop_back() = 0;

    // for object only
    virtual Table& operator[](const std::string& key) = 0; // insert if not existed (yaml-cpp does not insert)
    virtual Table* find(const std::string& key) const = 0;
    virtual bool has_member(const std::string& key) const = 0;
    virtual void erase(const std::string& key) = 0;

protected:
    enum class Type { JSON, YAML };

private:
    virtual Type type() const = 0;
    virtual bool is_equal(const Table& rhs) const = 0;
};

bool operator==(const Table& lhs, const Table& rhs);
bool operator!=(const Table& lhs, const Table& rhs);
} // namespace logtail
