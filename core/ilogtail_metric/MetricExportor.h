#pragma once

#include <string>

namespace logtail {


public:
    void pushMetrics();
    void pullMetrics();


private:
    void getILogtailInstanceMetrics()
    void getPluginMetrics()
    void getSubPluginMetrics()

}