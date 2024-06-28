#include "common/HttpServer.h"

#include "common/Logger.h"
#include "common/NetWorker.h"
#include "common/StringUtils.h"
#include "common/Chrono.h"

#include <sstream>

using namespace std;

namespace common
{
const int HttpServer::MAX_HTTP_HEADER = 8192;
HttpServerMsg HttpServer::CreateHttpServerResponse(const string &responseMsg,int responseCode)
{
    ostringstream sout;
    sout << "HTTP/1.1 " << responseCode << "\r\n"
        << "Content-Length: " << responseMsg.size() << "\r\n"
        << "Date: " << NowMillis() << "\r\n"
        << "Server: argusagent" << "\r\n\r\n";
    HttpServerMsg response;
    response.header = sout.str();
    response.body = responseMsg;
    return response;
}
RecvState HttpServer::ReceiveRequest(NetWorker *pNet, HttpServerMsg& req)
{
    if (pNet == nullptr) {
        return S_Error;
    }
    std::vector<char> buffer(MAX_HTTP_HEADER);
    // receive header
    string& data = req.header;
    if (data.find("\r\n\r\n") == string::npos)
    {
        size_t len = buffer.size();
        int ret = pNet->recv(&buffer[0], len);
        if (ret != 0 || len <= 0) {
            return S_Error;
        }
        // buffer[len] = 0;
        data.append(&buffer[0], len);
    }
    std::string::size_type pend = data.find("\r\n\r\n");
    if (pend == string::npos) {
        if (data.size() > MAX_HTTP_HEADER) {
            LogWarn("http request header too long: {} > {}", data.size(), MAX_HTTP_HEADER);
        }
        return data.size() > MAX_HTTP_HEADER? S_Error: S_Incomplete;
    }

    pend += 4;
    std::string header = data.substr(0, pend);
    std::string body = data.substr(pend);

    //check POST
    string::size_type plen = header.find("Content-Length: ");
    if (plen == string::npos) {
        plen = header.find("Content-length: ");
    }

    if (plen != string::npos)
    {
        plen += 16;
        string::size_type pos = header.find('\r', plen);
        size_t n = (pos != std::string::npos? (pos - plen): 32);
        std::string length = header.substr(plen, n);
        auto clen = convert<size_t>(length);

        //recv body
        while (body.size() < clen)
        {
            size_t len = buffer.size(); // sizeof(buffer) - 1;
            int ret = pNet->recv(&buffer[0], len);
            if (ret != 0 || len <= 0)
            {
                return S_Error;
            }
            // buffer[len] = 0;
            body.append(&buffer[0], len);// += buffer;
        }
    }
    if (!header.empty())
    {
        req.header = header;
        req.body = body;
    }

    return S_Complete;
}

int HttpServer::SendResponse(NetWorker *pNet, const HttpServerMsg& resp)
{
     if (pNet == nullptr) {
         return -1;
     }

    string data = resp.header + resp.body;
    const char *buff = data.c_str();
    size_t sendLen = 0;
    size_t leftLen = data.size();
    while (leftLen > 0)
    {
        size_t len = leftLen;
        int ret = pNet->send(buff + sendLen, len);
        if (ret != 0 || len <= 0)
        {
            LogError("send data failed! (already send: {})", sendLen);
            return -1;
        }
        leftLen -= len;
        sendLen += len;
    }
    return 0;
}

}