#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "TLVHandler.h"

namespace common {
    class NetWorker;

    struct HttpServerMsg {
        std::string header;
        std::string body;
    };

    class HttpServer {
    public:
        static HttpServerMsg CreateHttpServerResponse(const std::string &responseMsg, int responseCode);
        static RecvState ReceiveRequest(NetWorker *pNet, HttpServerMsg &req);
        static int SendResponse(NetWorker *pNet, const HttpServerMsg &resp);
        static const int MAX_HTTP_HEADER;
    };
}
#endif
