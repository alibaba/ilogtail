//
// Created by 韩呈杰 on 2023/5/25.
//

#ifndef AGENT_UNEXPECTED_RESOURCE_CONSUMPTION_H
#define AGENT_UNEXPECTED_RESOURCE_CONSUMPTION_H

#include <atomic>
#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include "common/Singleton.h"
#include "common/ThreadUtils.h"
#include "common/Common.h" // GetThisThreadId

struct ResourceConsumption {
    int ThreadId;   // 任务所在线程
    std::string TaskName; // 任务名称
    std::chrono::milliseconds Millis; // 耗时
    std::chrono::system_clock::time_point endTime; // 任务完成的时间

    std::string str() const;
};

#include "common/test_support"

class ResourceConsumptionRecorder {
public:
    class Task {
    private:

        friend class ResourceConsumptionRecorder;

        ResourceConsumptionRecorder *recorder = nullptr;
        int threadId = 0;
        std::string taskName;
        std::chrono::steady_clock::time_point startTime;
    public:
        Task(std::string n, ResourceConsumptionRecorder *r = nullptr);
        Task(std::string n, const std::chrono::steady_clock::time_point &, ResourceConsumptionRecorder *r = nullptr);
        ~Task();
    };

public:
#ifdef ENABLE_COVERAGE
    static constexpr const size_t topN = 10;
#else
    static constexpr const size_t topN = 20;
#endif

private:

    friend class Task;

    friend class common::Singleton<ResourceConsumptionRecorder>;

    // std::atomic<bool> open{true};

    mutable std::mutex mutex;
    std::unordered_map<std::string, ResourceConsumption> topTasks;
    std::unordered_map<Task *, Task> workingTask;

    ResourceConsumptionRecorder();
    static ResourceConsumption createRecord(const std::string &, const std::chrono::milliseconds &,
                                            int threadId, bool ended);
public:
    bool IsRecording() const {
        return true;
    }

    // void EnableRecording(bool enabled);
    bool AddTask(const std::string &taskName, const std::chrono::milliseconds &millis, int threadId);

    template<typename Rep, typename Period>
    bool AddTask(const std::string &taskName, const std::chrono::duration<Rep, Period> &dura, int threadId) {
        return AddTask(taskName, std::chrono::duration_cast<std::chrono::milliseconds>(dura), threadId);
    }

    size_t GetTopTasks(std::vector<ResourceConsumption> &tasks, int top = topN) const;
    size_t Count(bool ignoreWorking = true) const;

    std::string PrintTasks(int top = topN) const;
    static std::string PrintTasks(const std::vector<ResourceConsumption> &tasks);
};

#include "common/test_support"

typedef common::Singleton<ResourceConsumptionRecorder> AURCR;

#define CMS_CPU_PROFILE_3(LN, ...) ResourceConsumptionRecorder::Task __ ## LN ## _arugs_cpu_profiler_(__VA_ARGS__)
#define CMS_CPU_PROFILE_2(LN, ...) CMS_CPU_PROFILE_3(LN, __VA_ARGS__)
#define CpuProfile(...)            CMS_CPU_PROFILE_2(__LINE__, __VA_ARGS__)

#endif //AGENT_UNEXPECTED_RESOURCE_CONSUMPTION_H
