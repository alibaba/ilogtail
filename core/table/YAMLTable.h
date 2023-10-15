#pragma once

#include "table/Table.h"

#include "yaml-cpp/yaml.h"

namespace logtail {
class YAMLTable : public Table {
    friend bool operator==(const YAMLTable&, const YAMLTable&);
    friend bool operator!=(const YAMLTable&, const YAMLTable&);

public:
    ~YAMLTable() = default;
    YAMLTable(YAMLTable &&rhs) = default;
    YAMLTable& operator=(YAMLTable &&rhs) = default;

    bool parse(const std::string& content) override;
    std::string serialize() const override;
    bool is_null() const override;

private:
    Type type() const override { return sType; }
    bool is_equal(const Table& rhs) const override { return *this == static_cast<const YAMLTable&>(rhs); }

    static const Type sType = Type::YAML;
    YAML::Node mRoot;
};

bool operator==(const YAMLTable& lhs, const YAMLTable& rhs);
bool operator!=(const YAMLTable& lhs, const YAMLTable& rhs);
} // namespace logtail
