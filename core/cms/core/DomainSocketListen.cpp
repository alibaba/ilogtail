#include "DomainSocketListen.h"
#include "DomainSocketClient.h"

#include "common/CPoll.h"
#include "common/Logger.h"
#include "common/NetWorker.h"

using namespace std;
using namespace common;

namespace argus
{
DomainSocketListen::DomainSocketListen() {
    m_net = std::make_shared<NetWorker>("DomainSocketListen");
}
DomainSocketListen::~DomainSocketListen()
{
    SingletonPoll::Instance()->remove(this, __FUNCTION__);
    // for (auto pClient: m_clientList) {
    //     delete pClient;
    // }
    // m_clientList.Clear();
}
int DomainSocketListen::listen(const char *sockPath, uint16_t localPort, const std::shared_ptr<DomainSocketCollect> &c)
{
    m_net->setTimeout(3_s);
#if !HAVE_SOCKPATH
 // if (m_net.socket(AF_INET, SOCK_STREAM, APR_PROTO_TCP) != 0 || m_net.setSockOpt(APR_SO_REUSEADDR, 1) != 0 || m_net.setSockTimeOut(3_s) != 0) {
 //        LogError( "Create client listener failed");
 //        m_net.clear();
 //        return -1;
 //    }
 //    //绑定IP和端口
 //    //struct sockaddr_in sockaddIn;
 //    //sockaddIn.sin_family = AF_INET;
 //    //sockaddIn.sin_port = htons(localPort);
 //    //sockaddIn.sin_addr.S_un.S_addr = INADDR_ANY;
 //    //retBind = ::bind(this->m_net.getSock()->socketdes, (sockaddr *)&sockaddIn, sizeof(struct sockaddr_in));
	// retBind = m_net.bind(INADDR_ANY, localPort, "DomainSocketListen");
    int rv = m_net->listen("0.0.0.0", localPort, 30);
#else
    if (0 == access(sockPath, F_OK)) {
        remove(sockPath);
    }

    // if (m_net.socket(AF_UNIX, SOCK_STREAM, 0) != 0 || m_net.setSockOpt(APR_SO_REUSEADDR, 1) != 0 || m_net.setSockTimeOut(3_s) != 0) {
    //     LogError( "Create client listener failed");
    //     m_net.clear();
    //     return -1;
    // }
    // // struct sockaddr_un sockaddrun;
    // // sockaddrun.sun_family = AF_UNIX;
    // // strcpy(sockaddrun.sun_path, sockPath);
    // // retBind = ::bind(this->m_net.getSock()->socketdes, (sockaddr *) &sockaddrun, sizeof(struct sockaddr_un));
    // retBind = m_net.bind(sockPath, "DomainSocketListen");
    int rv = m_net->listenSockPath(sockPath, 30);
    if (0 == rv) {
        chmod(sockPath, 0777);
    }
#endif
    // if (0 != retBind) {
    //     cerr << "bind error" << strerror(errno) << endl;
    //     LogError("inner listener bind err[{}][{}]", errno, strerror(errno));
    //     return -1;
    // }
    // // if (0 != ::listen(this->m_net.getSock()->socketdes, 30)) {
    // if (0 != m_net.listen(30)) {
    //     cerr << "listen error" << strerror(errno) << endl;
    //     LogError("inner listener listen err[{}][{}]", errno, strerror(errno));
    //     return -1;
    // }
    if (rv != 0) {
        return -1;
    }
    // if(SingletonPoll::Instance()->add(m_net.getSock(), this) != 0)
    // {
    //     LogError( "Add client listener to poll failed");
    //     m_net.clear();
    //     return -1;
    // }
    m_collector = c;
#ifdef WIN32
    LogInfo("listen to local tcp port [{}] success",localPort);
#else
    LogInfo("listen to local file [{}] success",sockPath);
#endif
    return 0;
}
int DomainSocketListen::readEvent(const std::shared_ptr<PollEventBase> &myself)
{
    // Sync(m_clientList) {
    //     for (auto it = m_clientList.begin(); it != m_clientList.end();) {
    //         if ((*it)->isRunning()) {
    //             it++;
    //         } else {
    //             it = m_clientList.erase(it);
    //         }
    //     }
    // }}}
    std::shared_ptr<NetWorker> pNet;
    if (m_net->accept(pNet, "DomainSocketClient") != 0)
    {
        LogError( "Accept client failed");
        return -1;
    }

    LogInfo("Accept a new Domain-Socket client");
    // auto pClient = std::make_shared<DomainSocketClient>(pNet);
    // defer(pNet = nullptr); // 已加入pClient，退出时无需再释放
    auto c = m_collector.lock();
    if (!c) {
        LogError("DomainSocketCollect has been release");
        return -1;
    }
    if (SingletonPoll::Instance()->add<DomainSocketClient>(pNet, c) != 0) {
        LogError("Add a client to poll failed");
        // m_net.clear();
        // delete pClient;
        return -1;
    }
    // m_clientList.PushBack(pClient);
    return 0;
}
}
