//
// Created by 韩呈杰 on 2023/10/19.
//
#include <gtest/gtest.h>
#include "sic/system_information_collector_darwin.h"

#include <fmt/format.h>

class SicDarwinCollectorTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(SicDarwinCollectorTest, SicGetInterfaceConfigList) {
    DarwinSystemInformationCollector collector;

    SicInterfaceConfigList list;
    EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicGetInterfaceConfigList(list));
    EXPECT_FALSE(list.names.empty());

    int count = 0;
    for (const auto &name: list.names) {
        std::cout << fmt::format("if-name[{:2d}] {}", ++count, name) << std::endl;
    }
}

TEST_F(SicDarwinCollectorTest, SicGetInterfaceConfig) {
    DarwinSystemInformationCollector collector;

    SicInterfaceConfigList list;
    EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicGetInterfaceConfigList(list));
    EXPECT_FALSE(list.names.empty());

    for (const auto &name: list.names) {
        SicInterfaceConfig ifConfig;
        EXPECT_EQ(SIC_EXECUTABLE_SUCCESS, collector.SicGetInterfaceConfig(ifConfig, name));

        std::cout << ifConfig.name << "<" << ifConfig.type << ">: mtu " << ifConfig.mtu << std::endl
                  << "    ether " << ifConfig.hardWareAddr.str() << std::endl;
        if (ifConfig.address.family != SicNetAddress::SIC_AF_UNSPEC) {
            std::cout << "    inet " << ifConfig.address.str() << " netmask " << ifConfig.netmask.str()
                      << " broadcast " << ifConfig.broadcast.str() << std::endl;
        }
        if (ifConfig.address6.family != SicNetAddress::SIC_AF_UNSPEC) {
            std::cout << "    inet6 " << ifConfig.address6.str() << " prefixlen " << ifConfig.prefix6Length
                      << " scopeid " << fmt::format("0x{:x}", ifConfig.scope6) << std::endl;
        }

        EXPECT_NE(ifConfig.hardWareAddr.family, SicNetAddress::SIC_AF_UNSPEC);
    }
}
