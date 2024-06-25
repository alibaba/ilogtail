#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif

#include <cstdlib>
#include "common/TLVHandler.h"
#include "common/Logger.h"
#include "common/NetWorker.h"
#include "common/ScopeGuard.h"
#include "common/StringUtils.h"
#include "common/Arithmetic.h"

// #include <boost/asio.hpp> // 此处跟scope_guard.h的defer有冲突
#include <boost/asio/detail/socket_ops.hpp>  // host_to_network_short

using namespace common;
using namespace std;

TLVPackage::TLVPackage() {
    m_type = T_U8Json;
    reset();
}

TLVPackage::~TLVPackage() = default;

ProtoType TLVPackage::getType() const {
    return m_type;
}

void TLVPackage::setType(ProtoType type) {
    m_type = type;
}

const std::string &TLVPackage::getValue() const {
    return m_value;
}

void TLVPackage::setValue(const char *buf, int len) {
    m_value.assign(buf, len);
}

void TLVPackage::setValue(const std::string &value) {
    m_value = value;
}

void TLVPackage::reset() {
    m_value.clear();
    m_recvdLen = 0;
    m_sendLen = 0;
    m_totalLen = 0;
}

void TLVPackage::resetSendLen() {
    m_sendLen = 0;
}

std::string TLVPackage::serialize() const {
    using namespace boost::asio::detail::socket_ops;

    char buffer[TL_LEN] = {0};
    *reinterpret_cast<uint16_t *>(buffer) = host_to_network_short(m_type);
	// warning C4267: size_t => boost::asio::detail::u_long_type may lose data
    *reinterpret_cast<uint32_t *>(buffer + 2) = static_cast<uint32_t>(host_to_network_long(m_value.size()));

    string str;
    str.assign(buffer, TL_LEN);
    str += m_value;
    return str;
}

bool TLVPackage::deserialize(const std::string &buf) {
    if (buf.size() < TL_LEN) {
        return false;
    }

    using namespace boost::asio::detail::socket_ops;
    m_type = (ProtoType) network_to_host_short(*(uint16_t *) buf.data());
    auto len = network_to_host_long(*(uint32_t *) (buf.data() + 2));

    if (buf.size() < TL_LEN + len) {
        return false;
    }

    m_value = buf.substr(TL_LEN, len);
    return true;
}

/// ///////////////////////////////////////////////////////////////////////////////////////////////
///
RecvState TLVHandler::recvPackage(NetWorker *pNet, TLVPackage &package) {
    if (nullptr == pNet) {
        return S_Error;
    }

    if (package.m_recvdLen < TL_LEN) {
        size_t len = TL_LEN - package.m_recvdLen;
        char m_tl[TL_LEN] = {0};
        int ret = pNet->recv(m_tl + package.m_recvdLen, len);
        if (ret != 0 || len <= 0) {
            return S_Error;
        }
        package.m_recvdLen += len;
        if (package.m_recvdLen < TL_LEN) {
            return S_Incomplete;
        }

        //parse T and L
        using namespace boost::asio::detail::socket_ops;
        package.m_type = (ProtoType) network_to_host_short(*((uint16_t *) m_tl));
        package.m_totalLen = network_to_host_long(*(uint32_t *) (m_tl + 2)) + TL_LEN;

        if (package.m_totalLen == TL_LEN) {
            return S_Complete;
        }
    }
    if (package.m_totalLen > MAX_RECV_LEN) {
        LogWarn("package too long(size:{})!", convert(package.m_totalLen, true));
        return S_Error;
    }

    size_t buffLen = 512 * 1024;
    size_t surplus = [](size_t a, size_t b) { return a > b ? a - b : 0; }(package.m_totalLen, package.m_recvdLen);
    if (surplus < buffLen) {
        buffLen = surplus;
    }

    if (buffLen > 0) {
        // char *pValue = (char *) malloc(buffLen + 1);
        // if (pValue == nullptr) {
        //     LogError("malloc failed...");
        //     return S_Error;
        // }
        // defer(free(pValue));
        std::vector<char> pValue(buffLen + 1);
        // memset(pValue, 0, buffLen + 1);
        size_t len = buffLen;
        int ret = pNet->recv(&pValue[0], len);
        if (ret != 0 || len <= 0) {
            // free(pValue);
            return S_Error;
        }
        package.m_value.append(&pValue[0], len);
        // free(pValue);
        package.m_recvdLen += len;
    }

    if (package.m_recvdLen < package.m_totalLen) {
        LogDebug("already recv:{}, should recv:{}", package.m_recvdLen, package.m_totalLen);
        return S_Incomplete;
    } else {
        if (package.getType() == T_U8Json) {
            LogDebug("receive completely({}:{}) : type:T_U8Json, content:{}",
                     package.m_totalLen, package.getValue().size(), package.getValue());
        } else if (package.getType() == T_ProtoBuf) {
            LogDebug("receive completely({}:{}), protobuf", package.m_totalLen, package.getValue().size());
        } else if (package.getType() == T_PB_Ext) {
            LogDebug("receive completely({}:{}), pbext", package.m_totalLen, package.getValue().size());
        } else {
            LogWarn("receive completely, but type({}) not support", static_cast<int>(package.getType()));
        }
        return S_Complete;
    }
}

int TLVHandler::sendPackage(NetWorker *pNet, TLVPackage &package) {
    if (nullptr == pNet) {
        LogWarn("send error, not connected(send:{})!", package.m_sendLen);
        package.m_sendLen = 0;
        return -1;
    }

    const string str = package.serialize();
    const char *buff = str.data();
    size_t leftLen = str.size() - package.m_sendLen;

    while (leftLen > 0) {
        size_t len = leftLen;
        const int ret = pNet->send(buff + package.m_sendLen, len);
        if (ret != 0 || len <= 0) {
            LogError("send data failed! (already send:{})", package.m_sendLen);
            return ret;
        }
        leftLen -= len;
        package.m_sendLen += len;
    }

    if (package.getType() == T_U8Json) {
        LogDebug("send completely({}:{}): {}", str.size(), package.getValue().size(), package.getValue());
    } else if (package.getType() == T_ProtoBuf) {
        LogDebug("send completely({}:{}), protobuf", str.size(), package.getValue().size());
    } else if (package.getType() == T_PB_Ext) {
        LogDebug("send completely({}:{}), pbext", str.size(), package.getValue().size());
    }

    //reset send_len;
    package.m_sendLen = 0;

    return 0;
}
