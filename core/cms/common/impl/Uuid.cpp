//
// Created by 韩呈杰 on 2023/4/2.
//
#include "common/Uuid.h"

// header-only
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string NewUuid() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
}
