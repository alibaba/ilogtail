
#include "unittest/Unittest.h"
#include "unittest/UnittestHelper.h"
#include "observer/network/sources/pcap/PCAPWrapper.h"
#include "observer/network/NetworkObserver.h"
#include "observer/interface/helper.h"
#include "observer/network/protocols/utils.h"
namespace logtail {
class LocalFileWrapperUnittest : public ::testing::Test {
public:
    int OnPacketsToString(StringPiece packet) {
        printf("%s\n", PacketEventToString((void*)packet.data(), packet.length()).c_str());
        return 0;
    }
    static int OnObserverPacketsToString(std::vector<sls_logs::Log>& logs, Config* config) {
        std::string rst;
        for (auto& log : logs) {
            printf("%s \n", log.DebugString().c_str());
        }
        return 0;
    }
    void TestRunObserver() {
        auto config = NetworkConfig::GetInstance();
        config->mEnabled = true;
        config->mLocalFileEnabled = true;
        config->mFlushOutL4Interval = 10;
        config->mFlushOutL7Interval = 10;
        config->mFlushOutL7DetailsInterval = 10;
        // Please change the following lines when picking.
        config->mSaveToDisk = false;
        config->localPickConnectionHashId = -1;
        config->localPickPID = -1;
        config->localPickDstPort = -1;
        config->localPickSrcPort = -1;
        NetworkObserver* observer = NetworkObserver::GetInstance();
        observer->mSenderFunc=OnObserverPacketsToString;

        observer->ReloadSource();
        observer->StartEventLoop();
        sleep(30000);
    }
    void TestPrintData() {
        FILE* file = fopen64("ebpf.pick.connection.dump", "rb+");
        while (true) {
            uint32_t dataSize = 0;
            fread(&dataSize, 1, 4, file);
            if (dataSize != 0 && dataSize < 1024 * 1024) {
                char* buf = (char*)malloc(dataSize);
                if (dataSize != fread(buf, 1, dataSize, file)) {
                    LOG_ERROR(sLogger, ("read ebpf replay file failed", errno));
                    break;
                } else {
                    void* event = NULL;
                    int32_t len = 0;
                    BufferToPacketEvent(buf, dataSize, event, len);
                    if (event != NULL) {
                        PacketEventHeader* header = static_cast<PacketEventHeader*>((void*)event);
                        std::cout << "===========================" << std::endl;
                        std::cout << "PID: " << header->PID << " SockHash: " << header->SockHash << " SrcAddr"
                                  << SockAddressToString(header->SrcAddr) << " SrcPort: " << header->SrcPort
                                  << " DstAddr: " << SockAddressToString(header->DstAddr)
                                  << " DstPort: " << header->DstPort << std::endl;
                    }
                }
                free(buf);
            }
        }
    }
};
APSARA_UNIT_TEST_CASE(LocalFileWrapperUnittest, TestRunObserver, 0);
// APSARA_UNIT_TEST_CASE(LocalFileWrapperUnittest, TestPrintData, 0);
} // namespace logtail
int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}