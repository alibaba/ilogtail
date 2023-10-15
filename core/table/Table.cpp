#include "table/Table.h"

#include "table/JSONTable.h"
#include "table/YAMLTable.h"

using namespace std;

namespace logtail {
unique_ptr<Table> Table::CreateTable(const string& extension) {
    unique_ptr<Table> ptr;
    if (extension == "json") {
        ptr.reset(new JSONTable());
    } else if (extension == "yaml" || extension == "yml") {
        ptr.reset(new YAMLTable());
    }
    return ptr;
}

bool operator==(const Table& lhs, const Table& rhs) {
    return lhs.type() == rhs.type() && lhs.is_equal(rhs);
}

bool operator!=(const Table& lhs, const Table& rhs) {
    return !(lhs == rhs);
}
} // namespace logtail