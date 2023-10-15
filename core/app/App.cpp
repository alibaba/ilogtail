#include "app/App.h"

#include "gperftools/malloc_extension.h"

#include "app_config/AppConfig.h"
#include "common/Flags.h"
#include "config/ConfigDiff.h"
#include "config/watcher/ConfigWatcher.h"
#include "logger/Logger.h"
#include "monitor/Monitor.h"
#include "monitor/MetricExportor.h"
#include "Go_pipeline/LogtailPlugin.h"
#include "plugin/PluginRegistry.h"
#include "pipeline/PipelineManager.h"
#include "processor/LogProcess.h"
#ifdef __linux__
#include "observer/ObserverManager.h"
#endif
#include "sender/Sender.h"
#ifdef __ENTERPRISE__
#include "config/provider/EnterpriseConfigProvider.h"
#ifdef __linux__
#include "shennong/ShennongManager.h"
#include "streamlog/StreamLogManager.h"
#endif
#else
#include "config/provider/CommonConfigProvider.h"
#endif

#include "config_manager/ConfigManager.h"
#include "controller/EventDispatcher.h"
#include "event_handler/LogInput.h"
#include "checkpoint/CheckPointManager.h"
#include "file/FileServer.h"
#include "App.h"

DEFINE_FLAG_INT32(config_scan_interval, "seconds", 10);
DEFINE_FLAG_INT32(profiling_check_interval, "seconds", 60);
DEFINE_FLAG_INT32(tcmalloc_release_memory_interval, "force release memory held by tcmalloc, seconds", 300);

DECLARE_FLAG_BOOL(send_prefer_real_ip);
DECLARE_FLAG_BOOL(global_network_success);

namespace logtail {
#if defined(__ENTERPRISE__) && defined(_MSC_VER)
void InitWindowsSignalObject();
void SyncWindowsSignalObject();
#endif

void App::Start() {
#if defined(__ENTERPRISE__) && defined(_MSC_VER)
    InitWindowsSignalObject();
#endif
    // flusher_sls should always be loaded, since profiling will rely on this.
    Sender::Instance()->InitSender();

    LogtailMonitor::Instance()->InitMonitor();

    ConfigWatcher::GetInstance()->AddSource("local");
#ifdef __ENTERPRISE__
    EnterpriseConfigProvider::GetInstance()->Init("enterprise");
#else
    CommonConfigProvider::GetInstance()->Init("common");
#endif

    PluginRegistry::GetInstance()->LoadPlugins();
    PipelineManager::GetInstance()->Init();

#if defined(__ENTERPRISE__) && defined(__linux__)
    if (AppConfig::GetInstance()->ShennongSocketEnabled()) {
        ShennongManager::GetInstance()->Init();
    }
#endif

    // If in purage container mode, it means iLogtail is deployed as Daemonset, so plugin base should be loaded since
    // liveness probe relies on it.
    if (AppConfig::GetInstance()->IsPurageContainerMode()) {
        LogtailPlugin::GetInstance()->LoadPluginBase();
    }
    // Actually, docker env config will not work if not in purage container mode, so there is no need to load plugin
    // base. However, we still load it here for backward compatability.
    const char* dockerEnvConfig = getenv("ALICLOUD_LOG_DOCKER_ENV_CONFIG");
    if (dockerEnvConfig != NULL && strlen(dockerEnvConfig) > 0
        && (dockerEnvConfig[0] == 't' || dockerEnvConfig[0] == 'T')) {
        LogtailPlugin::GetInstance()->LoadPluginBase();
    }

    time_t curTime = 0, lastProfilingCheckTime = 0, lastTcmallocReleaseMemTime = 0, lastConfigCheckTime = 0,
           lastUpdateMetricTime = 0;
    while (true) {
        curTime = time(NULL);
        if (curTime - lastConfigCheckTime >= INT32_FLAG(config_scan_interval)) {
            ConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
            if (!diff.IsEmpty()) {
                PipelineManager::GetInstance()->UpdatePipelines(diff);
            }
            lastConfigCheckTime = curTime;
        }
        if (curTime - lastProfilingCheckTime >= INT32_FLAG(profiling_check_interval)) {
            LogFileProfiler::GetInstance()->SendProfileData();
            MetricExportor::GetInstance()->PushMetrics(false);
            lastProfilingCheckTime = curTime;
        }
#ifndef LOGTAIL_NO_TC_MALLOC
        if (curTime - lastTcmallocReleaseMemTime >= INT32_FLAG(tcmalloc_release_memory_interval)) {
            MallocExtension::instance()->ReleaseFreeMemory();
            lastTcmallocReleaseMemTime = curTime;
        }
#endif
        if (curTime - lastUpdateMetricTime >= 40) {
            CheckCriticalCondition(curTime);
            lastUpdateMetricTime = curTime;
        }
        if (LogtailGlobalPara::Instance()->GetSigtermFlag()) {
            LOG_INFO(sLogger, ("received SIGTERM signal", "exit process"));
            Exit();
        }
#if defined(__ENTERPRISE__) && defined(_MSC_VER)
        SyncWindowsSignalObject();
#endif
        // 过渡使用
        EventDispatcher::GetInstance()->DumpCheckPointPeriod(curTime);
        if (ConfigManager::GetInstance()->IsUpdateContainerPaths()) {
            FileServer::GetInstance()->Pause();
            FileServer::GetInstance()->Resume();
        }
    }
}

// TODO: undefined
void App::Exit() {
}

void App::CheckCriticalCondition(int32_t curTime) {
#ifdef __ENTERPRISE__
    int32_t lastGetConfigTime = ConfigManager::GetInstance()->GetLastConfigGetTime();
    // force to exit if config update thread is block more than 1 hour
    if (lastGetConfigTime > 0 && curTime - lastGetConfigTime > 3600) {
        LOG_ERROR(sLogger, ("last config get time is too old", lastGetConfigTime)("prepare force exit", ""));
        LogtailAlarm::GetInstance()->SendAlarm(
            LOGTAIL_CRASH_ALARM, "last config get time is too old: " + ToString(lastGetConfigTime) + " force exit");
        LogtailAlarm::GetInstance()->ForceToSend();
        sleep(10);
        _exit(1);
    }
#endif
    // if network is fail in 2 hours, force exit (for ant only)
    // work around for no network when docker start
    if (BOOL_FLAG(send_prefer_real_ip) && !BOOL_FLAG(global_network_success)
        && curTime - ConfigManager::GetInstance()->GetStartTime() > 7200) {
        LOG_ERROR(sLogger, ("network is fail", "prepare force exit"));
        LogtailAlarm::GetInstance()->SendAlarm(
            LOGTAIL_CRASH_ALARM,
            "network is fail since " + ToString(ConfigManager::GetInstance()->GetStartTime()) + " force exit");
        LogtailAlarm::GetInstance()->ForceToSend();
        sleep(10);
        _exit(1);
    }

    int32_t lastDaemonRunTime = Sender::Instance()->GetLastDeamonRunTime();
    if (lastDaemonRunTime > 0 && curTime - lastDaemonRunTime > 3600) {
        LOG_ERROR(sLogger, ("last sender daemon run time is too old", lastDaemonRunTime)("prepare force exit", ""));
        LogtailAlarm::GetInstance()->SendAlarm(LOGTAIL_CRASH_ALARM,
                                               "last sender daemon run time is too old: " + ToString(lastDaemonRunTime)
                                                   + " force exit");
        LogtailAlarm::GetInstance()->ForceToSend();
        sleep(10);
        _exit(1);
    }

    int32_t lastSendTime = Sender::Instance()->GetLastSendTime();
    if (lastSendTime > 0 && curTime - lastSendTime > 3600 * 12) {
        LOG_ERROR(sLogger, ("last send time is too old", lastSendTime)("prepare force exit", ""));
        LogtailAlarm::GetInstance()->SendAlarm(LOGTAIL_CRASH_ALARM,
                                               "last send time is too old: " + ToString(lastSendTime) + " force exit");
        LogtailAlarm::GetInstance()->ForceToSend();
        sleep(10);
        _exit(1);
    }

    LogtailMonitor::Instance()->UpdateMetric("last_send_time", GetTimeStamp(lastSendTime, "%Y-%m-%d %H:%M:%S"));
}

} // namespace logtail