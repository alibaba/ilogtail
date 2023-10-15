#pragma once

#include "table/Table.h"

#include "json/json.h"

namespace logtail {
class JSONTable : public Table {
    friend bool operator==(const JSONTable&, const JSONTable&);
    friend bool operator!=(const JSONTable&, const JSONTable&);

public:
    ~JSONTable() = default;
    JSONTable(JSONTable &&rhs) = default;
    JSONTable& operator=(JSONTable &&rhs) = default;

    bool parse(const std::string& content) override;
    std::string serialize() const override;
    bool is_null() const override;

private:
    Type type() const override { return sType; }
    bool is_equal(const Table& rhs) const override { return *this == static_cast<const JSONTable&>(rhs); }

    static const Type sType = Type::JSON;
    Json::Value mRoot;
};

bool operator==(const JSONTable& lhs, const JSONTable& rhs);
bool operator!=(const JSONTable& lhs, const JSONTable& rhs);
} // namespace logtail
