#ifndef _DOMAIN_SOCKET_COLLECT_H_
#define _DOMAIN_SOCKET_COLLECT_H_

#include <string>
#include <map>

#include "common/Singleton.h"

#include "TaskManager.h"
#include "DomainSocketListen.h"

namespace argus {
#include "common/test_support"
    class DomainSocketCollect {
    public:
        DomainSocketCollect();

        ~DomainSocketCollect();

        static int start(const std::shared_ptr<DomainSocketCollect> &);

        int collectData(int type, std::string &msg);

    private:
        std::shared_ptr<std::map<int, ReceiveItem>> mPrevReceiveItems;
        std::map<int, ReceiveItem> mReceiveItemMap;
        common::InstanceLock m_itemLock;
        std::shared_ptr<DomainSocketListen> mDomainSocketListen;
        std::string mSocketPath;
		uint16_t mPort = 0;
    };
#include "common/test_support"
}
#endif