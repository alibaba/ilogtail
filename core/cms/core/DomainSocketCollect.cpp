#include "DomainSocketCollect.h"
#include "argus_manager.h"
#include "common/Logger.h"
#include "common/CPoll.h"
#include "common/Config.h"
#include "common/Chrono.h"

using namespace std;
using namespace common;

namespace argus
{
DomainSocketCollect::DomainSocketCollect(){
    Config *cfg = SingletonConfig::Instance();
    mDomainSocketListen = std::make_shared<DomainSocketListen>();
    mSocketPath = cfg->getReceiveDomainSocketPath();
    mPort = cfg->getReceivePort();
}
DomainSocketCollect::~DomainSocketCollect(){
    // if(mDomainSocketListen!=NULL)
    // {
    //     delete mDomainSocketListen;
    //     mDomainSocketListen = NULL;
    // }
    if (mDomainSocketListen && mDomainSocketListen->m_net) {
        mDomainSocketListen->m_net->shutdown();
    }
}


int DomainSocketCollect::start(const std::shared_ptr<DomainSocketCollect> &c){
    LogDebug("sockPath: {}, port: {}", c->mSocketPath, c->mPort);
    int rv = c->mDomainSocketListen->listen(c->mSocketPath.c_str(), c->mPort, c);
    if (0 == rv) {
        SingletonPoll::Instance()->add(c->mDomainSocketListen, c->mDomainSocketListen->m_net);
    }
    return rv;
}

int DomainSocketCollect::collectData(int type,string &msg){
    TaskManager *pTask = SingletonTaskManager::Instance();
    m_itemLock.lock();
    if (pTask->ReceiveItems().Get(mPrevReceiveItems)) {
        mReceiveItemMap = *mPrevReceiveItems;
    }
    // if(pTask->ReceiveItemChanged())
    // {
    //     pTask->GetReceiveItems(mReceiveItemMap);
    // }
    if(mReceiveItemMap.find(type)==mReceiveItemMap.end()){
        //丢弃数据
        LogWarn("skip msg,type={}",type);
        m_itemLock.unlock();
        return -1;
    }
    ReceiveItem collectItem =mReceiveItemMap[type];
    m_itemLock.unlock();
    // int64_t timestamp = NowSeconds();
    LogDebug("type={},msgSize={},msg={}", type, msg.size(), msg);
    if (auto pModuleManager = SingletonArgusManager::Instance()->getChannelManager()) {
        pModuleManager->sendMsgToNextModule(collectItem.name, collectItem.outputVector,
                                            std::chrono::Now<BySeconds>(), 0, msg, false, "");
    }
    return 0;
}
}
