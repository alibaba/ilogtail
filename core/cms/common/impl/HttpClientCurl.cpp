// #ifdef HTTP_CLIENT_WITH_CURL
#include "common/HttpClient.h"

#include <vector>

#include <curl/curl.h>
#include <openssl/err.h>

#include "common/ScopeGuard.h"
#include "common/Common.h" // GetThisThreadId
#include "common/StringUtils.h"
#include "common/Logger.h"

#ifdef ENABLE_COVERAGE
bool curlVerbose = true;
#else
bool curlVerbose = false;
#endif

namespace common {
    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CurlInitializer
    class CurlInitializer {
        static std::once_flag onceFlag;
        static std::mutex *mutex;
    public:
        static void Initialize() {
            std::call_once(onceFlag, [&]() {
                curl_global_init(CURL_GLOBAL_ALL);
                CreateSslLock();
                LogInfo("libCurl {} Initialized", LIBCURL_VERSION);
            });
        }

        static void Destroy() {
            curl_global_cleanup();
            SslLockCleanup();
        }

        static void LockingFunction(int mode, int type, const char *file, int line) {
            // 根据第1个参数mode来判断是加锁还是解锁
            // 第2个参数是数组下标
            if (mode & CRYPTO_LOCK) {
                mutex[type].lock();
            } else {
                mutex[type].unlock();
            }
        }

        static unsigned long IdFunction() {
            return GetThisThreadId();
        }

        static int CreateSslLock() {
            // 申请空间，锁的个数是：CRYPTO_num_locks()，
            mutex = new std::mutex[CRYPTO_num_locks()];
            // 设置回调函数，获取当前线程id
            CRYPTO_set_id_callback(IdFunction);
            // 设置回调函数，加锁和解锁
            CRYPTO_set_locking_callback(LockingFunction);
            return 0;
        }

        static void SslLockCleanup() {
            delete []mutex;
            mutex = nullptr;
        }
    };

    std::once_flag CurlInitializer::onceFlag;
    std::mutex *CurlInitializer::mutex;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HttpClient

    // // Doc: https://developer.mozilla.org/en-US/docs/Web/HTTP # mozilla关于http协议的详细解说
    // // Doc: https://curl.se/libcurl/c/CURLOPT_DEBUGFUNCTION.html # CURLOPT_DEBUGFUNCTION explained
    // int curlDebugCallbak(CURL* , curl_infotype type, char* data, size_t size, void*) {
    //     //const char* text = nullptr;
    //     //(void)handle; /* prevent compiler warning */
    //     //(void)clientp;

    //     switch (type) {
    //     case CURLINFO_TEXT:
    //     case CURLINFO_HEADER_OUT:
    //     case CURLINFO_HEADER_IN:
    //         //fputs("== Info: ", stderr);
    //         fwrite(data, size, 1, stderr);
    //         break;

    //     // case CURLINFO_DATA_OUT:
    //     //    text = "=> Send data";
    //     //    break;
    //     // case CURLINFO_SSL_DATA_OUT:
    //     //    text = "=> Send SSL data";
    //     //    break;
    //     // case CURLINFO_DATA_IN:
    //     //    text = "<= Recv data";
    //     //    break;
    //     // case CURLINFO_SSL_DATA_IN:
    //     //    text = "<= Recv SSL data";
    //     //   break;
    //     default:
    //         break;
    //     }
    //     //if (text != nullptr) {
    //     //    std::cerr << text << std::endl;
    //     //}

    //     return 0;
    // }

    // 关于libCurl的稳定性问题，官方并没有给于强力保证: https://blog.csdn.net/delphiwcdj/article/details/18284429
    //
    int HttpClient::performHttpImpl(HttpMethod method, const HttpRequest &request, HttpResponse &response,
            const std::string& cipher, const std::string& netInterface) {
        auto res = static_cast<CURLcode>(-1);
        CurlInitializer::Initialize();
        CURL *curl = curl_easy_init();

        response.resCode = 0;

#define curl_setopt(Key, Value) res = curl_easy_setopt(curl, (Key), (Value)); \
       if (res != CURLE_OK) {                                                 \
            response.errorMsg = curl_easy_strerror(res);                      \
            return res;                                                       \
       }

        if (curl) {
            defer(curl_easy_cleanup(curl));

            curl_setopt(CURLOPT_URL, request.url.c_str());
            if (!request.unixSocketPath.empty()) {
                curl_setopt(CURLOPT_UNIX_SOCKET_PATH, request.unixSocketPath.c_str());
            }

            const char *methodName = "GET";
            if (method == HttpMethod::GET) {
                curl_setopt(CURLOPT_HTTPGET, 1L);
            } else if (method == HttpMethod::POST) {
                methodName = "POST";
                // https://curl.se/libcurl/c/curl_easy_setopt.html:
                // CURLOPT_HTTPPOST Deprecated option Multipart formpost HTTP POST.
                // curl_setopt(CURLOPT_POST, 1L); // 显式指定为POST，但不设置POSTFIELDS会导致卡死(主要原因是，此时curl会在read call等待数据)
                if (request.body.empty()) {
                    curl_setopt(CURLOPT_CUSTOMREQUEST, "POST"); // 这种处理方式是curl程序的处理方式，这样不会增加额外的header
                } else {
                    curl_setopt(CURLOPT_POSTFIELDS, request.body.c_str());
                    curl_setopt(CURLOPT_POSTFIELDSIZE, (long)request.body.size()); // not necessary
                }
            } else if (method == HttpMethod::HEAD) {
                methodName = "HEAD";
                curl_setopt(CURLOPT_NOBODY, 1L);
            }

            struct curl_slist *head = nullptr;
            defer(curl_slist_free_all(head));
            for (const auto &i: request.httpHeader) {
                std::string curHeaderItem = TrimSpace(i);
                auto tmp = curl_slist_append(head, curHeaderItem.c_str());
                if (tmp == nullptr) {
                    LogError("curl_slist_append({}) failed, [{}]{}", curHeaderItem, methodName, request.url);
                } else {
                    head = tmp;
                }
            }
            if (head) {
                curl_setopt(CURLOPT_HTTPHEADER, head);
            }

            curl_setopt(CURLOPT_ACCEPT_ENCODING, ""); // 所有curl支持的编码
            if (curlVerbose) {
                curl_setopt(CURLOPT_VERBOSE, 1L);
            }
            if (!netInterface.empty()) {
                curl_setopt(CURLOPT_INTERFACE, netInterface.c_str()); // 指定出口网卡，适用于主、次网卡的情况
            }

            // 超时
            curl_setopt(CURLOPT_TIMEOUT, static_cast<long>(request.timeout > 0 ? request.timeout : 5));
            curl_setopt(CURLOPT_CONNECTTIMEOUT,
                             static_cast<long>(request.timeout > 0 && request.timeout < 5 ? request.timeout : 5));

            curl_setopt(CURLOPT_USERAGENT, "curl/" LIBCURL_VERSION);

            const bool isHttps = HasPrefix(request.url, "https://", true);
            // if (isHttps) {
                curl_setopt(CURLOPT_SSL_VERIFYPEER, 0L); // 当redirect时，如果目标是https则同样不verify
                curl_setopt(CURLOPT_SSL_VERIFYHOST, 0L);
                //curl_setopt(CURLOPT_SSL_VERIFYSTATUS, 0L);
            // }
            if (isHttps && !cipher.empty()) {
                curl_setopt(CURLOPT_SSL_CIPHER_LIST, cipher.data());
            }
            if (!request.proxy.empty()) {
                curl_setopt(CURLOPT_PROXY, request.proxy.c_str());

                constexpr char httpFlag[] = "http\0\0"; // 7 == strlen("http://")
                constexpr size_t httpFlagStrLen = 4;
                static_assert(7 == sizeof(httpFlag), "unexpect sizeof(HttpFlags)");
                static_assert(httpFlag[httpFlagStrLen] == '\0', "unexpect httpFlagStrLen");
                if (request.proxy.size() > sizeof(httpFlag) && HasPrefix(request.proxy, httpFlag, true)) {
                    if (!request.user.empty() && !request.password.empty()) {
                        std::string userPassword = request.user + ":" + request.password;
                        curl_setopt(CURLOPT_PROXYUSERPWD, userPassword.c_str());
                    }
                    // https/2, since curl 8.1.0
#ifdef CURLPROXY_HTTPS2
                    char c = request.proxy[httpFlagStrLen];
                    if ((c == 's' || c == 'S') && request.proxySchemeVersion == "2") {
                        curl_setopt(CURLOPT_PROXYTYPE, CURLPROXY_HTTPS2);
                    }
#endif
                }
            }
            // 支持重定向，最多重定向20次
            curl_setopt(CURLOPT_MAXREDIRS, 20L);
            curl_setopt(CURLOPT_FOLLOWLOCATION, 1L);
            // 多线程设置(禁止访问超时的时候抛出超时信号)
            curl_setopt(CURLOPT_NOSIGNAL, 1L);
            // 结果写入
            curl_setopt(CURLOPT_WRITEFUNCTION, &HttpClient::HttpCallBack);
            curl_setopt(CURLOPT_WRITEDATA, (void *) &response);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                response.errorMsg = curl_easy_strerror(res);
            } else {
                long resCode = 0;
                res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resCode);
                response.resCode = static_cast<int>(resCode);
            }
        } else {
            response.errorMsg = "unexpected error: curl init failed";
        }
#undef curl_setopt

        return res;
    }

    bool HttpClient::IsTimeout(int resCode) {
        return resCode == CURLE_OPERATION_TIMEDOUT;
    }

    std::string HttpClient::StrError(int code) {
        return curl_easy_strerror(static_cast<CURLcode>(code));
    }

#if defined(DISABLE_BOOST_URL)
    std::string HttpClient::UrlEncode(const std::string &src) {
        // curl版的url encode
        std::string result = src;
        CurlInitializer::Initialize();
        CURL *curl = curl_easy_init();
        if (curl) {
            defer(curl_easy_cleanup(curl));
            char *output = curl_easy_escape(curl, src.data(), static_cast<int>(src.size()));
            if (output) {
                defer(curl_free(output));
                result = output;
            }
        }
        return result;
    }

    std::string HttpClient::UrlDecode(const std::string &src) {
        std::string ret;

        ret = src;
        CurlInitializer::Initialize();
        CURL *curl = curl_easy_init();
        if(curl) {
            defer(curl_easy_cleanup(curl));
            int decodelen = 0;
            char *decoded = curl_easy_unescape(curl, src.data(), static_cast<int>(src.size()), &decodelen);
            if(decoded) {
                defer(curl_free(decoded));
                ret = decoded;
            }
        }
        return ret;
    }
#endif
} // namespace common
// #endif // HTTP_CLIENT_WITH_CURL
