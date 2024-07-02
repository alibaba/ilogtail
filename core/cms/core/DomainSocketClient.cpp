#include "DomainSocketClient.h"
#include "DomainSocketCollect.h"
#include "common/Logger.h"
#include "common/NetWorker.h"

using namespace std;
using namespace common;


namespace argus {
    DomainSocketClient::DomainSocketClient(const std::shared_ptr<common::NetWorker> &pNet,
                                           const std::shared_ptr<DomainSocketCollect> &fnCollector)
            : m_net(pNet), m_fnCollector(fnCollector) {
        // this->m_run=1;
        this->m_PacketNum = 0;
    }

    DomainSocketClient::~DomainSocketClient() {
        LogInfo("~DomainSocketClient");
        // m_net.reset();
        // if (m_net->getSock() != nullptr)
        // {
        //     SingletonPoll::Instance()->delSocket(m_net->getSock());
        // }
        // delete m_net;
    }

    apr_socket_t *DomainSocketClient::getSock() const {
        return m_net ? m_net->getSock() : nullptr;
    }

    int DomainSocketClient::readEvent(const std::shared_ptr<PollEventBase> &) {
        RecvState state;
        if ((m_package.mRecvSize < MSG_HDR_LEN)) {
            state = ReadDSPacketHdr();
            if (state == S_Error) {
                //处理错误
                LogError("receive data from client failed");
                // doClose();
                m_net->shutdown();
                return -1;
            } else if (state == S_Incomplete) {
                return 1;
            }
        }
        //头部已经读取
        state = ReadDSPacketData();
        if (state == S_Error) {
            //处理错误
            LogError("recv data from client failed");
            // doClose();
            m_net->shutdown();
            return -1;
        } else if (state == S_Incomplete) {
            return 1;
        }
        m_PacketNum++;
        LogInfo("receive one package,type={},totalNum={}", m_package.mPacketType, m_PacketNum);
        string msgStr;
        if (!m_package.mPacket.empty()) {
            msgStr.assign(&m_package.mPacket[0], m_package.mPacket.size());
        }
        if (auto collector = m_fnCollector.lock()) {
            collector->collectData(m_package.mPacketType, msgStr);
        }
        m_package = DomainSocketPacket{}; // .reset(); // reset packet buffer state
        return 0;
    }

    RecvState DomainSocketClient::ReadDSPacketHdr() {
        if (!m_net || !*m_net) {
            return S_Error;
        }
        if (m_package.mRecvSize >= MSG_HDR_LEN) {
            return S_Complete;
        }
        size_t recvHdrLen = MSG_HDR_LEN - m_package.mRecvSize;
        int ret = m_net->recv(m_package.mPacketHdr + m_package.mRecvSize, recvHdrLen);
        if (ret != 0 || recvHdrLen <= 0) {
            return S_Error;
        }
        m_package.mRecvSize += recvHdrLen;
        if (m_package.mRecvSize < MSG_HDR_LEN) {
            return S_Incomplete;
        } else {
            //头部已经读取
            auto *msgHdrPtr = (MessageHdr *) (m_package.mPacketHdr);
            if ((msgHdrPtr->len > 0) && (msgHdrPtr->len < MAX_DOMAIN_SOCKET_PACKAGE_SIZE)) {
                m_package.mPacket.resize(msgHdrPtr->len);
                // m_package.mPacketSize = msgHdrPtr->len;
                m_package.mPacketType = msgHdrPtr->type;
                return S_Complete;
            } else {
                LogError("the package size ({}) is too large", msgHdrPtr->len);
                return S_Error;
            }
        }
    }

    RecvState DomainSocketClient::ReadDSPacketData() {
        if (!m_net || !*m_net) {
            return S_Error;
        }
        size_t recvLen = m_package.mPacket.size() - m_package.mRecvSize + MSG_HDR_LEN;
        int ret = m_net->recv(&m_package.mPacket[0] + m_package.mRecvSize - MSG_HDR_LEN, recvLen);
        if (ret != 0 || recvLen <= 0) {
            return S_Error;
        }
        m_package.mRecvSize += recvLen;
        return (m_package.mRecvSize == m_package.mPacket.size() + MSG_HDR_LEN) ? S_Complete : S_Incomplete;
    }

// int DomainSocketClient::doClose()
// {
//     if(m_net==NULL||m_net->getSock()==NULL){
//         return -1;
//     }
//     apr_socket_t* pSock = m_net->getSock();
//     if (SingletonPoll::Instance()->delSocket(pSock) != 0)
//     {
//         LogError( "Delete domain-socket client from poll failed");
//     }else{
//         LogInfo("close domain-socket client successful");
//     }
//     m_net->close();
//     // this->m_run=0;
//     return 0;
// }
}