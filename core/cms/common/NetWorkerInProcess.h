//
// Created by 韩呈杰 on 2024/1/30.
//

#ifndef ARGUSAGENT_NETWORKERMOCK_H
#define ARGUSAGENT_NETWORKERMOCK_H
#ifdef ENABLE_COVERAGE

#include <algorithm> // std::min
#include <functional>
#include "NetWorker.h"
#include "common/SyncQueue.h"

#ifdef min
#	undef min
#endif
struct RecvBuf {
    std::string received;

    int recv(char *data, size_t &len, const std::function<std::string()> &supply) {
        if (received.empty() && len > 0 && supply) {
            received = supply();
        }

        len = std::min({len, received.size()});
        memcpy(data, received.c_str(), len);
        received = received.substr(len);

        return 0;
    }
};

// 进程内的NetWorker通信
class InProcessNetWorker : public common::NetWorker {
    mutable SyncQueue<bool, Nonblock> accept_{10};
    SyncQueue<std::string> queueSend{10};
    SyncQueue<std::string> queueRecv{10};
    RecvBuf recvBuf;
public:
    bool enableSend = true;
    bool valid = true;

    explicit InProcessNetWorker(const std::string &scenario = {}) : NetWorker(scenario) {
    }

    bool isValid() const override {
        return valid;
    }

    int send(const char *data, size_t &len) override {
        if (!enableSend) {
            return -1;
        }
        queueSend << std::string(data, len);
        accept_ << true;

        return 0;
    }

    int recv(char *data, size_t &len, bool) override {
        return recvBuf.recv(data, len, [this]() {
            std::string s;
            queueRecv >> s;
            return s;
        });
    }

    int shutdown() override {
        queueSend.Close();
        queueRecv.Close();
        accept_.Close();
        return 0;
    }

    int accept(std::shared_ptr<NetWorker> &net, const std::string &) override {
        if (accept_.Wait(300_s)) {
            auto clone = std::make_shared<InProcessNetWorker>();
            clone->queueRecv = queueSend;
            clone->queueSend = queueRecv;
            net = clone;
            return 0;
        }
        return -1;
    }

    std::shared_ptr<NetWorker> accept() {
        std::shared_ptr<NetWorker> client;
        accept(client, "");
        return client;
    }
};

#endif // !ENABLE_COVERAGE
#endif //ARGUSAGENT_NETWORKERMOCK_H
