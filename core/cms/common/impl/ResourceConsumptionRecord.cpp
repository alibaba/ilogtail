//
// Created by 韩呈杰 on 2023/5/25.
//
#include "common/ResourceConsumptionRecord.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include <fmt/format.h>

#include "common/Chrono.h"

using namespace std::chrono;

std::string ResourceConsumption::str() const {
    return fmt::format("thread: {:6d}, millis: {:4d}, taskName: {}", ThreadId, ToMillis(Millis), TaskName);
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
constexpr const size_t ResourceConsumptionRecorder::topN;
ResourceConsumptionRecorder::ResourceConsumptionRecorder() = default;

// void ResourceConsumptionRecorder::EnableRecording(bool enable) {
//     if (enable != open) {
//         std::lock_guard<std::mutex> guard(mutex);
//         if (enable != open) {
//             topTasks.clear();
//             open = enable;
//         }
//     }
// }

size_t ResourceConsumptionRecorder::Count(bool ignoreWorking) const {
    std::lock_guard<std::mutex> guard(mutex);
    return topTasks.size() + (ignoreWorking ? 0 : workingTask.size());
}

size_t ResourceConsumptionRecorder::GetTopTasks(std::vector<ResourceConsumption> &tasks, int top) const {
    {
        std::lock_guard<std::mutex> guard(mutex);

        tasks.reserve(topTasks.size() + workingTask.size());
        for (const auto &it: topTasks) {
            tasks.push_back(it.second);
        }
        // 正在运行，尚未结束的任务
        for (const auto &it: workingTask) {
            const Task &task = it.second;
            auto dura = std::chrono::duration_cast<decltype(ResourceConsumption::Millis)>(
                    std::chrono::steady_clock::now() - task.startTime);
            tasks.push_back(createRecord(task.taskName + "(working)", dura, task.threadId, false));
        }
    }

    // 按耗时倒序
    std::sort(tasks.begin(), tasks.end(), [](const ResourceConsumption &a, const ResourceConsumption &b) {
        return a.Millis > b.Millis;
    });
    if (top > 0 && tasks.size() > size_t(top)) {
        tasks.resize(top);
    }

    return tasks.size();
}

ResourceConsumption ResourceConsumptionRecorder::createRecord(const std::string &taskName,
                                                              const std::chrono::milliseconds &millis,
                                                              int threadId, bool ended) {
    ResourceConsumption record;
    // 时长不足1ms的任务不必记录
    record.TaskName = taskName;
    record.Millis = millis;
    record.ThreadId = threadId;
    if (ended) {
        record.endTime = std::chrono::system_clock::now();
    }
    return record;
}

bool ResourceConsumptionRecorder::AddTask(const std::string &taskName,
                                          const std::chrono::milliseconds &millis, int threadId) {
    bool enabled = IsRecording();
    if (enabled) {
        ResourceConsumption record = createRecord(taskName, millis, threadId, true);

        std::lock_guard<std::mutex> guard(mutex);
        topTasks[taskName] = record;

        // 超出topN，则踢出最小耗时的记录
        if (topTasks.size() > topN) {
            const ResourceConsumption *min = &topTasks.begin()->second;
            for (const auto &it: topTasks) {
                const auto &cur = it.second;
                if (min->Millis > cur.Millis || (min->Millis == cur.Millis && min->endTime > cur.endTime)) {
                    min = &cur;
                }
            }
            topTasks.erase(min->TaskName);
        }
    }
    return enabled;
}

std::string ResourceConsumptionRecorder::PrintTasks(int top) const {
    std::vector<ResourceConsumption> tasks;
    this->GetTopTasks(tasks, top);
    return PrintTasks(tasks);
}

std::string ResourceConsumptionRecorder::PrintTasks(const std::vector<ResourceConsumption> &tasks) {
    std::stringstream ss;
    size_t count = 0;
    int width = tasks.size() <= 10 ? 1 : 2;
    for (const auto &it: tasks) {
        ss << fmt::format("[{:{}d}] ", count++, width) << it.str() << std::endl;
    }
    return ss.str();
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Task
ResourceConsumptionRecorder::Task::Task(std::string n, const std::chrono::steady_clock::time_point &t,
                                        ResourceConsumptionRecorder *r) :
        threadId((int) GetThisThreadId()), taskName(std::move(n)), startTime(t) {

    ResourceConsumptionRecorder *tmp = (r ? r : AURCR::Instance());
    if (tmp) {
        std::lock_guard<std::mutex> guard(tmp->mutex);
        tmp->workingTask.emplace(this, *this);
    }
    recorder = tmp;
}

ResourceConsumptionRecorder::Task::Task(std::string n, ResourceConsumptionRecorder *r) :
        Task(std::move(n), std::chrono::steady_clock::now(), r) {
}

ResourceConsumptionRecorder::Task::~Task() {
    if (this->recorder) {
        {
            std::lock_guard<std::mutex> guard(this->recorder->mutex);
            // 如果workingTask里存储的task.recorder不为null，则此处会产生死锁。
            this->recorder->workingTask.erase(this);
        }
        this->recorder->AddTask(taskName, std::chrono::steady_clock::now() - this->startTime, this->threadId);
        this->recorder = nullptr;
    }
}
