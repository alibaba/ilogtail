#ifndef _TLV_HANDLER_H_
#define _TLV_HANDLER_H_

#include <string>

namespace common
{
    class NetWorker;

enum ProtoType
{
    T_Binary   = 0,
    T_U8Json   = 1,
    T_ProtoBuf = 2,
    T_PB_Ext   = 3,
};

enum RecvState
{
    S_Error      = 0,
    S_Incomplete = 1,
    S_Complete   = 2,
};

constexpr size_t TL_LEN = sizeof(uint16_t) + sizeof(uint32_t);

#include "test_support"
class TLVPackage
{
    friend class TLVHandler;
public:
     TLVPackage();
    ~TLVPackage();

    ProtoType  getType() const;
    void setType(ProtoType type);

    const std::string& getValue() const;
    void setValue(const std::string& value);
    void setValue(const char* buf, int len);

    std::string serialize() const;
    bool deserialize(const std::string& buf);

    void reset();

    void resetSendLen();

private:
    ProtoType m_type = T_U8Json;
    std::string m_value;

    size_t m_recvdLen = 0;
    size_t m_sendLen = 0;
    size_t m_totalLen = 0;
};
#include "test_support"

class TLVHandler
{
public:
    static const size_t MAX_RECV_LEN = 64 * 1024 * 1024;;

    static RecvState recvPackage(NetWorker *pNet, TLVPackage& package);
    //-1 error, 0 ok, other apr_status
    static int sendPackage(NetWorker *pNet, TLVPackage& package);
};

}
#endif
