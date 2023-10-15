#pragma once

#include <cstdint>

namespace logtail {
class App {
public:
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    static App* GetInstance() {
        static App instance;
        return &instance;
    }

    void Start();

private:
    App() = default;
    ~App() = default;

    void Exit();
    void CheckCriticalCondition(int32_t curTime);
};
} // namespace logtail
