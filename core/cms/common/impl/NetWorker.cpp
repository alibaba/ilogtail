#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6
#endif

#include <apr-1/apr_network_io.h>

#include "common/NetWorker.h"

#include "common/Logger.h"
#include "common/SocketUtils.h"
#include "common/Chrono.h"
#include "common/BoostStacktraceMt.h"
#include "common/Common.h"  // IPv4String
#include "common/Defer.h"
#include "common/StringUtils.h"

#include <thread>

#if !defined(WIN32)

#include <resolv.h>
#include <netinet/in.h>  // res_init

#endif

using namespace std;
using namespace common;

#if !defined(WIN32)
#   if !APR_HAVE_SOCKADDR_UN
#       error "expect macro APR_HAVE_SOCKADDR_UN defined"
#   endif
#endif

#if HAVE_SOCKPATH != APR_HAVE_SOCKADDR_UN
#	error "maco HAVE_SOCKPATH != APR_HAVE_SOCKADDR_UN"
#endif
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
static apr_status_t set_timeout(const std::string &, apr_socket_t *m_sock,
                                const boost::optional<std::chrono::microseconds> &optTimeout,
                                bool forConnect);
static const char *getIPAddr(apr_socket_t *sock, apr_interface_e which, const char *&addr, const char *defaultAdd);
static uint16_t getPort(apr_socket_t *sock, apr_interface_e which);

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
static std::shared_ptr<apr_socket_t> makeSharedSocket(const std::string& scenario, apr_socket_t *p) {
    return {p, [scenario](apr_socket_t *ptr) {
#ifdef ENABLE_COVERAGE
        const char *local = nullptr;
        const char *remote = nullptr;
        LogDebug("{}apr_socket_close([{}:{}, {}:{}])", scenario,
                 getIPAddr(ptr, APR_LOCAL, local, "0.0.0.0"), getPort(ptr, APR_LOCAL),
                 getIPAddr(ptr, APR_REMOTE, remote, "ANY"), getPort(ptr, APR_REMOTE));
#endif
        apr_socket_close(ptr);
    }};
}

static int createSocket(const std::string &scenario, apr_pool_t *pool,
                        std::shared_ptr<apr_socket_t> &sock, const char *&local, const char *&remote,
                        int family, int type, int proto) {
    apr_socket_t *sockp = nullptr;
    apr_status_t rv = apr_socket_create(&sockp, family, type, proto, pool);
    if (rv != APR_SUCCESS) {
        LogWarn("{}{} error: ({}){}", scenario, SocketDesc("apr_socket_create", family, type, proto),
                rv, NetWorker::ErrorString(rv));
    } else {
        sock = makeSharedSocket(scenario, sockp);
        local = remote = nullptr;
    }
    return rv;
}

// 不允许直接使用，只能使用特化版本
template<EnumSocketType T>
int createSocket(const std::string &scenario, apr_pool_t *pool, std::shared_ptr<apr_socket_t> &sock,
                 const char *&local, const char *&remote) = delete;

template<>
int createSocket<TCP>(const std::string &scenario, apr_pool_t *pool, std::shared_ptr<apr_socket_t> &sock,
                      const char *&local, const char *&remote) {
    return createSocket(scenario, pool, sock, local, remote, APR_INET, SOCK_STREAM, APR_PROTO_TCP);
}

template<>
int createSocket<UDP>(const std::string &scenario, apr_pool_t *pool, std::shared_ptr<apr_socket_t> &sock,
                      const char *&local, const char *&remote) {
    return createSocket(scenario, pool, sock, local, remote, APR_INET, SOCK_DGRAM, APR_PROTO_UDP);
}

template<>
int createSocket<ICMP>(const std::string &scenario, apr_pool_t *pool, std::shared_ptr<apr_socket_t> &sock,
                       const char *&local, const char *&remote) {
    return createSocket(scenario, pool, sock, local, remote, APR_INET, SOCK_RAW, IPPROTO_ICMP);
}

template<EnumSocketType>
const char *name() = delete;

template<>
constexpr const char *name<TCP>() {
    return "TCP";
}

template<>
constexpr const char *name<UDP>() {
    return "UDP";
}

template<>
constexpr const char *name<ICMP>() {
    return "ICMP";
}

#if HAVE_SOCKPATH

template<>
int createSocket<SOCK_PATH>(const std::string &scenario, apr_pool_t *pool, std::shared_ptr<apr_socket_t> &sock,
                            const char *&local, const char *&remote) {
    return createSocket(scenario, pool, sock, local, remote, AF_UNIX, SOCK_STREAM, 0);
}

template<>
constexpr const char *name<SOCK_PATH>() {
    return "SOCK_PATH";
}

#endif

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
std::string NetWorker::ErrorString(apr_status_t rv) {
    std::vector<char> buf(1024);
    return rv == APR_SUCCESS? "Success": apr_strerror(rv, &buf[0], buf.size());
}

NetWorker::NetWorker(const char *op, const std::chrono::microseconds &timeout) :
        scenario(op ? std::string{op} + " " : "") {
    apr_pool_create(&m_p, nullptr);
    setTimeout(timeout);
}

NetWorker::~NetWorker() {
    clear();
    apr_pool_destroy(m_p);
}

#ifdef ENABLE_COVERAGE

int NetWorker::createTcpSocket() {
    return createSocket<TCP>(scenario, m_p, m_sock, localAddr, remoteAddr);
}

#endif

static std::string to_string(const apr_sockaddr_t &sa) {
    std::string s;
    if (sa.servname != nullptr && *sa.servname != '\0') {
        s = sa.servname;
    } else if (sa.ipaddr_len == 4) {
        s = IPv4String(*static_cast<uint32_t *>(sa.ipaddr_ptr));
    } else if (sa.ipaddr_len == 16) {
        s = IPv6String(static_cast<uint32_t *>(sa.ipaddr_ptr));
    }

    return fmt::format("{}:{}", s, sa.port);
}

template<EnumSocketType T>
int NetWorker::connectInner(apr_sockaddr_t *sa, bool reuseSocket) {
    apr_status_t rv = APR_EINVALSOCK;
    if (sa) {
        rv = APR_SUCCESS;
        // 这里的reuseSocket指的是复用NetWorker的socket，而不是协议层面的复用。
        if (!reuseSocket || !this->m_sock) {
            rv = createSocket<T>(scenario, m_p, m_sock, localAddr, remoteAddr);
            if (APR_SUCCESS == rv) {
                rv = this->setOpt(APR_SO_REUSEADDR, 1);
            }
        }
        if (APR_SUCCESS == rv) {
            set_timeout(scenario, m_sock.get(), m_timeout, true);
            rv = apr_socket_connect(m_sock.get(), sa);
        }
    }
    if (rv != APR_SUCCESS) {
        LogWarn("{}apr_socket_connect<{}>({}) error: ({}){}",
                scenario, name<T>(), (sa ? to_string(*sa) : "nil"), rv, ErrorString(rv));
    }
    return rv;
}

apr_sockaddr_t *NetWorker::createAddr(const char *hostname, uint16_t port) {
    apr_sockaddr_t *sa = nullptr;
    if (hostname && hostname[0]) {
        apr_status_t rv = apr_sockaddr_info_get(&sa, hostname, APR_INET, port, 0, m_p);
        if (rv != APR_SUCCESS) {
            sa = nullptr;
            LogWarn("{} {}({}:{}) error: ({}){}", scenario, __FUNCTION__, hostname, port, rv, ErrorString(rv));
#if !defined(WIN32)
            LogInfo("try res_init() ...");
            res_init();
#endif
        }
    } else {
        LogInfo("{}{} error: hostname is null or empty, caller[5]: \n{}", scenario, __FUNCTION__,
                boost::stacktrace::mt::to_string(boost::stacktrace::stacktrace(0, 5)));
    }
    return sa;
}

int NetWorker::connect(const char *hostOrIP, uint16_t remotePort, bool tcp, bool reuseSocket) {
    if (tcp) {
        return connectInner<TCP>(createAddr(hostOrIP, remotePort), reuseSocket);
    } else {
        return connectInner<UDP>(createAddr(hostOrIP, remotePort), reuseSocket);
    }
}

#if HAVE_SOCKPATH

apr_sockaddr_t *NetWorker::createAddrForSockPath(const char *sockPath) {
    apr_sockaddr_t *sa = nullptr;
    apr_status_t rv = apr_sockaddr_info_get(&sa, sockPath, APR_UNIX, 0, 0, m_p);
    if (rv != APR_SUCCESS || sa == nullptr) {
        LogWarn("{} apr_sockaddr_info_get({}) error, ({}){}", scenario, sockPath, rv, ErrorString(rv));
    }

    // sa->family = APR_UNIX;
    // strncpy(sa->sa.unx.sun_path, sockPath, sizeof(sa->sa.unx.sun_path));
    return sa;
}

int NetWorker::connectSockPath(const char *sockPath) {
    return connectInner<SOCK_PATH>(createAddrForSockPath(sockPath), false);
}

#endif

int NetWorker::accept(std::shared_ptr<NetWorker> &pNet, const std::string &sce) {
    pNet = std::make_shared<NetWorker>(sce.empty() ? (scenario + "Client") : sce);
    set_timeout(scenario, m_sock.get(), m_timeout, false);
    apr_socket_t *remote = nullptr;
    apr_status_t rv = apr_socket_accept(&remote, m_sock.get(), pNet->getPool());
    if (rv != APR_SUCCESS) {
        LogWarn("{}apr_socket_accept at {} error: ({}){}", scenario, local(), rv, ErrorString(rv));
    } else {
        pNet->m_sock = makeSharedSocket(pNet->scenario, remote);
    }
    return rv;
}

int NetWorker::shutdown() {
    apr_status_t rv = APR_SUCCESS;
    if (m_sock) {
        std::string descriptor = fmt::format("socket[{}, {}]", local(), remote());
        LogDebug("{}shutdown std::shared_ptr({}).use_count() = {}", scenario, descriptor, m_sock.use_count());
        rv = apr_socket_shutdown(m_sock.get(), APR_SHUTDOWN_READWRITE);
        if (rv != APR_SUCCESS) {
            LogWarn("{}apr_socket_shutdown({}, READ | WRITE) error: ({}){}", scenario, descriptor, rv, ErrorString(rv));
        }
    }
    return rv;
}

void NetWorker::close() {
    if (m_sock) {
        if (m_sock.use_count() > 1) {
            LogDebug("close std::shared_ptr(socket[{}]).use_count() = {}", toString(), m_sock.use_count());
        }
        m_sock.reset();
    }
}

int NetWorker::sendTo(apr_sockaddr_t *where, int32_t flags, const char *buf, size_t &len) {
    apr_socket_t *sock = m_sock.get();
    if (sock == nullptr) {
        return -1;
    }
    apr_status_t rv = apr_socket_sendto(sock, where, flags, buf, &len);
    if (rv != APR_SUCCESS) {
        LogWarn("{}apr_socket_sendto({}, {} bytes) error: ({}){}",
                scenario, to_string(*where), len, rv, ErrorString(rv));
    }
    return rv;
}

int NetWorker::send(const char *buf, size_t &len) {
    apr_socket_t *sock = m_sock.get();
    if (sock == nullptr) {
        return -1;
    }
    set_timeout(scenario, sock, m_timeout, false);
    apr_status_t rv = apr_socket_send(sock, buf, &len);
    if (rv != APR_SUCCESS) {
        LogWarn("{}apr_socket_send({}) error: ({}){}", scenario, remote(), rv, ErrorString(rv));
    }
    return rv;
}

int NetWorker::recv(char *buf, size_t &len, bool loopWait) {
    apr_socket_t *sock = m_sock.get();
    if (sock == nullptr) {
        return -1;
    }
    set_timeout(scenario, sock, m_timeout, false);

    size_t rdLen = 0;
    size_t buffLen = len;

    defer(len = rdLen); // 退出时，返回实际读取的数据量

    apr_status_t rv;
    do {
        size_t tlen = buffLen - rdLen;
        rv = apr_socket_recv(sock, buf + rdLen, &tlen);
        rdLen += tlen;
        if (tlen <= 0 && APR_EAGAIN != rv) {
            // len = rdLen;
            LogDebug("{}apr_socket_recv([from]{}) error: ({}){}", scenario, remote(), rv, ErrorString(rv));
            return rv;
        }
        if (APR_EAGAIN == rv && buffLen > rdLen && loopWait) {
            std::this_thread::sleep_for(1_us); // std::chrono::microseconds{1});
        }
    } while (APR_EAGAIN == rv && buffLen > rdLen && loopWait);

    // len = rdLen;
    return rv;
}

int NetWorker::bind(apr_sockaddr_t *sa) {
    apr_status_t rv = apr_socket_bind(m_sock.get(), sa);
    if (rv != APR_SUCCESS) {
        LogWarn("{} bind({}) error: ({}){}", scenario, to_string(*sa), rv, ErrorString(rv));
    }
    return rv;
}

int NetWorker::setOpt(int32_t opt, int32_t on) {
    apr_status_t rv = apr_socket_opt_set(m_sock.get(), opt, on);
    if (rv != APR_SUCCESS) {
        LogWarn("{} set_socket_opt(socket[{}, {}], {}, {}) error: ({}){}",
                scenario, local(), remote(), opt, on, rv, ErrorString(rv));
    }
    return rv;
}

static apr_status_t set_timeout(const std::string &scenario, apr_socket_t *m_sock,
                                const boost::optional<std::chrono::microseconds> &optTimeout,
                                bool /** forConnect */) {
    // See: http://dev.ariel-networks.com/apr/apr-tutorial/html/apr-tutorial-13.html#ss13.4  # 《blocking vs. non-blocking socket》
    // 此处处理为：
    // timeout == 0: 不阻塞
    // timeout <  0: 永久阻塞
    // timeout >  0: 超时阻塞
    apr_status_t rv = APR_SUCCESS;
    if (optTimeout.has_value()) {
        const auto timeout = ToMicros(*optTimeout);
// #ifdef WIN32
//         int32_t nonblockValue = (timeout == 0 || forConnect ? 1 : 0);
//         rv = apr_socket_opt_set(m_sock, APR_SO_NONBLOCK, nonblockValue); // 在windows下无效，始终使用的system-timeout
// #endif
        rv = apr_socket_timeout_set(m_sock, (timeout == 0 ? -1 : timeout));
        if (rv != APR_SUCCESS) {
            LogWarn("{}apr_socket_timeout_set({}) error: ({}) {}",
                    scenario, *optTimeout, rv, NetWorker::ErrorString(rv));
        }
    }
    return rv;
}

void NetWorker::clear() {
    close();
    apr_pool_clear(m_p);
}

static apr_sockaddr_t *getSockAddr(apr_socket_t *sock, apr_interface_e which) {
    apr_sockaddr_t *sockAddr = nullptr;
    if (sock != nullptr) {
        apr_socket_addr_get(&sockAddr, which, sock);
    }
    return sockAddr;
}

static const char *getIPAddr(apr_socket_t *sock, apr_interface_e which, const char *&addr, const char *defaultAddr) {
    if (addr == nullptr) {
        char *ip = nullptr;
        if (auto *sockAddr = getSockAddr(sock, which)) {
            // 每调一次apr_sockaddr_ip_get, 都会在pool中产生一个char[16]的缓存，因此此处缓存到addr里，避免内存泄漏
            apr_sockaddr_ip_get(&ip, sockAddr);
        }
        addr = (ip == nullptr ? defaultAddr : ip);
    }
    return addr;
}

static uint16_t getPort(apr_socket_t *sock, apr_interface_e which) {
    const auto *sockAddr = getSockAddr(sock, which);
    return sockAddr ? sockAddr->port : 0;
}

std::string NetWorker::getLocalAddr() const {
    return getIPAddr(m_sock.get(), APR_LOCAL, localAddr, "0.0.0.0");
}

uint16_t NetWorker::getLocalPort() const {
    return getPort(m_sock.get(), APR_LOCAL);
}

std::string NetWorker::local() const {
    return fmt::format("{}:{}", getLocalAddr(), getLocalPort());
}

std::string NetWorker::getRemoteAddr() const {
    return getIPAddr(m_sock.get(), APR_REMOTE, remoteAddr, "");
}

uint16_t NetWorker::getRemotePort() const {
    return getPort(m_sock.get(), APR_REMOTE);
}

std::string NetWorker::remote() const {
    std::string s = getRemoteAddr();
    if (!s.empty() || getRemotePort() > 0) {
        s += ":" + convert(getRemotePort());
    }
    return s;
}

std::string NetWorker::toString() const {
    std::string s;
    s = local();
    std::string r = remote();
    if (!r.empty()) {
        s += ", " + r;
    }
    s = fmt::format("[{}]", s);

    return s;

}

int NetWorker::listen(int32_t backlog) {
    apr_status_t rv = apr_socket_listen(m_sock.get(), backlog);
    if (rv != APR_SUCCESS) {
        LogWarn("{}listen at {} error: ({}){}", scenario, local(), rv, ErrorString(rv));
    }
    return rv;
}

int NetWorker::listen(apr_sockaddr_t *sa, bool isSockPath, int32_t backlog) {
    apr_status_t rv = sa? APR_SUCCESS: APR_EINVALSOCK;
    defer(
            if (sa && rv != APR_SUCCESS) {
                LogError("create {} listener at {} failed. ({}){}",
                         scenario, (sa? to_string(*sa): "<null>"), rv, ErrorString(rv));
                clear();
            }
    );

    if (sa) {
        if (rv == APR_SUCCESS) {
            auto fn = createSocket<TCP>;
#if HAVE_SOCKPATH
            if (isSockPath) {
                fn = createSocket<SOCK_PATH>;
            }
#endif
            rv = fn(scenario, m_p, m_sock, localAddr, remoteAddr);
        }
        if (rv == APR_SUCCESS) {
            rv = this->setOpt(APR_SO_REUSEADDR, 1);
        }
        if (rv == APR_SUCCESS) {
            rv = set_timeout(scenario, m_sock.get(), m_timeout, false);
        }
        if (rv == APR_SUCCESS) {
            rv = this->bind(sa);
        }
        if (rv == APR_SUCCESS && backlog > 0) {
            rv = this->listen(backlog);
        }
    }

    return rv == APR_SUCCESS ? 0 : -1;
}

int NetWorker::listen(const char *hostOrIP, uint16_t localPort, int32_t backlog) {
    return listen(createAddr(hostOrIP, localPort), false, backlog);
}

#if HAVE_SOCKPATH

int NetWorker::listenSockPath(const char *sockPath, int backlog) {
    return listen(createAddrForSockPath(sockPath), true, backlog);
}

#endif

int NetWorker::openIcmp(int32_t bufSize) {
    apr_status_t rv = APR_SUCCESS;
    defer(if (rv != APR_SUCCESS) { clear(); });

    if (rv == APR_SUCCESS) {
        rv = createSocket<ICMP>(scenario, m_p, m_sock, localAddr, remoteAddr);
    }
    if (rv == APR_SUCCESS && bufSize > 0) {
        rv = this->setOpt(APR_SO_RCVBUF, bufSize);
    }
    if (rv == APR_SUCCESS) {
        rv = this->setOpt(APR_SO_NONBLOCK, 1);
    }
    if (rv == APR_SUCCESS) {
        rv = set_timeout(scenario, m_sock.get(), m_timeout, false);
    }

    return rv == APR_SUCCESS ? 0 : -1;
}
// int NetWorker::setSockKeepAlive(int idle, int intvl, int count)
// {
// #ifdef LINUX
// #ifdef __APPLE__
// #   define SOL_TCP      IPPROTO_TCP
// #   define TCP_KEEPIDLE TCP_KEEPALIVE
// #endif
//     int keepAlive = 1;
//     auto client_fd = m_sock->socketdes;
//     setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
//     setsockopt(client_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&idle, sizeof(idle));
//     setsockopt(client_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&intvl, sizeof(intvl));
//     setsockopt(client_fd, SOL_TCP, TCP_KEEPCNT, (void *)&count, sizeof(count));
// #endif
//
// #ifdef WIN32
//     auto client_fd = m_sock->socketdes;
//     tcp_keepalive settings;
//     settings.onoff = 1;
//     settings.keepaliveinterval = intvl * 1000; // interval of a probe when failed, in ms
//     settings.keepalivetime = idle * 1000;   // every X ms, default is 2 hours
//     DWORD nBytes=0;
//     WSAIoctl(client_fd, SIO_KEEPALIVE_VALS, &settings, sizeof(settings), nullptr, 0, &nBytes, nullptr, nullptr);
// #endif
//
//     return 0;
// }
