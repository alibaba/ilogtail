#ifndef _CONTAINER_INFO_PROVIDER_H_
#define _CONTAINER_INFO_PROVIDER_H_

#include "InstanceLock.h"
#include "Singleton.h"
#include <string>
#include <map>
#include <vector>
#include <unordered_map>

namespace common {
    enum ContainerEnv {
        POUCH_ENV = 0,
        DOCKER_ENV = 1,
        NO_CONTAIN_SUPPORT = 2,
    };
    struct ContainerInfo {
        std::string id;
        std::string pid;
        std::string sn;
    };
    struct ContainerCgroupInfo {
        std::string id;
        std::string cpuacc;
        std::string memory;
        std::string blkio;
        std::string net_cls;
    };
    struct ContainerMountInfo {
        std::string source;//Source字段
        std::string dest;//Destination字段
    };
    struct ContainerNodePath {
        ContainerMountInfo mountInfo;
        std::string dockerFile;
        std::string file;
    };
    struct ContainerInspectInfo {
        std::string id;//对应Id字段
        std::string name;//对应Name字段,去掉前缀"/"
        std::vector <ContainerMountInfo> mounts;
        std::unordered_map <std::string, std::string> labelMap;
        ContainerCgroupInfo cgroupInfo;
        ContainerInfo containerInfo;
    };

#include "test_support"
class ContainerInfoProvider {
public:
    ContainerInfoProvider();
    void UpdateConfig(const std::string &unixSocketPath, const std::string &url, int64_t interval);
    bool getLiveContainerInfo(std::map <std::string, ContainerInfo> &dockerInfoMap);
    bool getContainerElement(const std::string &cmd, const std::string &id, std::string &result);
    int GetContainInspectInfos(std::vector <ContainerInspectInfo> &containerInspectInfos, bool forceUpdate = false);
    std::string GetContainerNodePath(const std::vector <std::string> &dockerIds, const std::string &dockerPath);
    //保留更详细的信息。
    bool GetContainerNodePath(const std::vector <std::string> &dockerIds, const std::string &dockerPath,
                              ContainerNodePath &containerNodePath);
    bool GetContainerInfo(const std::string &containerId, ContainerInfo &containerInfo);
private:
    bool checkContainEnv();
    bool getLiveContainerIds(std::vector <std::string> &liveContainerIds);
    bool updateLiveContainerInfo();
    bool printContainerInfo(std::map <std::string, ContainerInfo> &dockerInfoMap);
    bool ParseContainerInfo(const std::string &json, ContainerInfo &containerInfo);
    bool GetContainerCgroupInfo(const std::string &file, ContainerCgroupInfo &containerCgroupInfo);
    int64_t mLastContainerInfoUpdateTime;
    InstanceLock mContainerInfoMapLock;
    ContainerEnv mContainerEnv;
    std::map <std::string, ContainerInfo> mContainerInfoMap;
    int ParseDockerInspectInfos(const std::string &json, std::vector <ContainerInspectInfo> &containerInspectInfos);
    //for docker api with unix domain socket
    std::string mUnixSocketPath;
    std::string mUrl;
    std::string mContainerUrl;
    int64_t mInterval;
    int64_t mLastContainInspectInfoUpdate;
    std::vector <ContainerInspectInfo> mContainerInspectInfos;
    InstanceLock mContainerInspectInfoLock;
};
#include "test_support"

    typedef Singleton<ContainerInfoProvider> SingletonContainerInfoProvider;
}
#endif
