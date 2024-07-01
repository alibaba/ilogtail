#ifndef ARGUS_COMMON_NET_WORKER_H
#define ARGUS_COMMON_NET_WORKER_H

#include <string>
#include <memory>  // std::shared_ptr
#include <boost/optional.hpp>
#include "common/ChronoLiteral.h"

struct apr_sockaddr_t;
struct apr_pool_t;
struct apr_socket_t;

namespace common {
    enum EnumSocketType {
        TCP,
        UDP,
        ICMP,
#if HAVE_SOCKPATH
        SOCK_PATH,
#endif
    };

#include "test_support"
class NetWorker {
public:
    explicit NetWorker(const char *op, const std::chrono::microseconds &timeout = 3_s);

    explicit NetWorker(const std::string &s, const std::chrono::microseconds &timeout = 3_s)
            : NetWorker(s.c_str(), timeout) {
    }

    VIRTUAL ~NetWorker();

    VIRTUAL bool isValid() const {
        return (bool) m_sock;
    }

    explicit operator bool() const {
        return isValid();
    }

    // 禁止拷贝
    NetWorker(const NetWorker &) = delete;
    NetWorker &operator=(const NetWorker &) = delete;

public:
    /// Server /////////////////////////////////////////////////////////////////////////////////////////////////////
    // 当backlog<=0时，则只进行到bind
    VIRTUAL int listen(const char *hostOrIP, uint16_t localPort, int32_t backlog);

    int listen(const std::string &hostOrIP, uint16_t localPort, int32_t backlog) {
        return listen(hostOrIP.c_str(), localPort, backlog);
    }

#if HAVE_SOCKPATH
    VIRTUAL int listenSockPath(const char *sockPath, int32_t backlog);
#endif
    VIRTUAL int accept(std::shared_ptr<NetWorker> &net, const std::string &scenario = {});

    int accept(std::shared_ptr<NetWorker> &net, const char *szScenario) {
        return accept(net, std::string(szScenario ? szScenario : ""));
    }

    int bind(const std::string &hostOrIP, uint16_t localPort) {
        return listen(hostOrIP, localPort, -1);
    }

    int bind(const char *hostOrIP, uint16_t localPort) {
        return listen(hostOrIP, localPort, -1);
    }

#if HAVE_SOCKPATH

    int bindSockPath(const char *sockPath) {
        return listenSockPath(sockPath, -1);
    }

#endif
    /// Client /////////////////////////////////////////////////////////////////////////////////////////////////////
    // reuseSocket: 使用之前已初始化好的m_sock，不再重新生成一个
    VIRTUAL int connect(const char *hostOrIP, uint16_t remotePort, bool tcp = true, bool reuseSocket = false);

    int connect(const std::string &hostOrIP, uint16_t remotePort, bool tcp = true, bool reuseSocket = false) {
        return connect(hostOrIP.c_str(), remotePort, tcp, reuseSocket);
    }

    template<EnumSocketType ST>
    int connect(const char *hostOrIP, uint16_t remotePort, bool reuseSocket = false) {
        return connect(hostOrIP, remotePort, (TCP == ST), reuseSocket);
    }

    template<EnumSocketType ST>
    int connect(const std::string &hostOrIP, uint16_t remotePort, bool reuseSocket = false) {
        return connect<ST>(hostOrIP.c_str(), remotePort, reuseSocket);
    }

    template<EnumSocketType ST>
    int connect(const std::string &localHost, uint16_t localPort, const std::string &rHost, uint16_t rPort) {
        int rv = bind(localHost, localPort);
        if (rv == 0) {
            rv = connect<ST>(rHost, rPort, true);
        }
        return rv;
    }

#if HAVE_SOCKPATH
    VIRTUAL int connectSockPath(const char *sockPath);

    int connectSockPath(const std::string &sockPath) {
        return connectSockPath(sockPath.c_str());
    }

#endif
#ifdef ENABLE_COVERAGE
    /// Special purpose ////////////////////////////////////////////////////////////////////////////////////////////
    VIRTUAL int createTcpSocket();
#endif

    /// ICMP(for ping) /////////////////////////////////////////////////////////////////////////////////////////////
    VIRTUAL int openIcmp(int32_t bufSize = 64 * 1024);

    VIRTUAL int send(const char *buf, size_t &len);
    VIRTUAL int recv(char *buf, size_t &len, bool loopWait = true);

    VIRTUAL int sendTo(apr_sockaddr_t *where, int32_t flags, const char *buf, size_t &len);

    template<typename Rep, typename Period>
    int setTimeout(const std::chrono::duration<Rep, Period> &t) {
        m_timeout = std::chrono::duration_cast<std::chrono::microseconds>(t);
        return 0;
    }

    VIRTUAL int setOpt(int32_t opt, int32_t on);

    VIRTUAL std::string getLocalAddr() const;
    VIRTUAL uint16_t getLocalPort() const;
    VIRTUAL std::string local() const;

    VIRTUAL std::string getRemoteAddr() const;
    VIRTUAL uint16_t getRemotePort() const;
    VIRTUAL std::string remote() const;

    VIRTUAL int shutdown();

    apr_socket_t *getSock() const {
        return m_sock.get();
    }

    apr_pool_t *getPool() const {
        return m_p;
    }

    VIRTUAL void clear();

    static std::string ErrorString(int rv);
    VIRTUAL std::string toString() const;

    VIRTUAL apr_sockaddr_t *createAddr(const char *hostname, uint16_t port);

private:
#if HAVE_SOCKPATH
    apr_sockaddr_t *createAddrForSockPath(const char *sockPath);
#endif

    template<EnumSocketType>
    int connectInner(apr_sockaddr_t *sa, bool reuseSocket);

    int listen(int32_t backlog);
    int listen(apr_sockaddr_t *sockAddr, bool isSockPath, int32_t backlog);

    int bind(apr_sockaddr_t *sa);
    void close();

private:
    apr_pool_t *m_p = nullptr;

    std::shared_ptr<apr_socket_t> m_sock;  // 允许新建socket
    mutable const char *localAddr = nullptr;
    mutable const char *remoteAddr = nullptr;

    std::string scenario;
    boost::optional<std::chrono::microseconds> m_timeout;
};
#include "test_support"

}

#endif
