#include "unittest/Unittest.h"
#include "unittest/UnittestHelper.h"
#include "network/protocols/buffer.h"

namespace logtail {
class ContinueBufferTest : public ::testing::Test {
    static void BufferAdd(Buffer& buffer, const std::string& str, int32_t pos, int32_t time) {
        buffer.Add(pos, time, str.data(), str.size(), str.size());
    }

public:
    void TestAdd() {
        Buffer buffer(15, 15, 15);
        auto piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "");
        BufferAdd(buffer, "0123", 0, 0);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123");
        piece = buffer.Head();
        BufferAdd(buffer, "45", 4, 4);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "012345");
        BufferAdd(buffer, "89", 8, 8);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "012345");
        BufferAdd(buffer, "67", 6, 6);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123456789");
        BufferAdd(buffer, "abcde", 10, 10);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123456789abcde");
        BufferAdd(buffer, "fghij", 15, 15);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "56789abcdefghij");
        BufferAdd(buffer, "st", 28, 28);
        BufferAdd(buffer, "qr", 26, 26);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "fghij");
        BufferAdd(buffer, "op", 24, 24);
        BufferAdd(buffer, "mn", 22, 22);
        BufferAdd(buffer, "kl", 20, 20);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "fghijklmnopqrst");
        buffer.RemovePrefix(5);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "klmnopqrst");
        BufferAdd(buffer, "12345", 100, 100);
        buffer.Trim();
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "12345");

        BufferAdd(buffer, "small", 30, 30);
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "12345");
    }


    void TestRemovePrefixAndTrim() {
        Buffer buffer(15, 15, 15);
        BufferAdd(buffer, "0123", 0, 0);
        BufferAdd(buffer, "abcd", 10, 10);
        auto piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123");
        buffer.RemovePrefix(2);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "23");
        buffer.RemovePrefix(1);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "3");
        buffer.RemovePrefix(1);
        buffer.Trim();
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "abcd");
        buffer.RemovePrefix(-1);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "abcd");
    }


    void TestTimestamp() {
        Buffer buffer(15, 15, 15);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(0), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(20), -1);
        BufferAdd(buffer, "0123", 0, 0);
        BufferAdd(buffer, "4567", 4, 4);
        buffer.RemovePrefix(1);
        auto piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "1234567");
        APSARA_TEST_EQUAL(buffer.GetTimestamp(0), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(1), 0);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(2), 0);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(3), 0);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(4), 4);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(5), 4);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(6), 4);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(7), 4);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(8), -1);
    }

    void TestTimestampGap() {
        Buffer buffer(15, 15, 15);
        BufferAdd(buffer, "0123", 0, 0);
        BufferAdd(buffer, "abcd", 10, 10);
        auto piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123");
        APSARA_TEST_EQUAL(buffer.GetTimestamp(0), 0);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(3), 0);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(4), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(9), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(10), 10);
        buffer.RemovePrefix(2);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(0), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(1), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(2), 0);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(9), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(10), 10);

        buffer.RemovePrefix(2);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(0), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(9), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(10), 10);

        buffer.Trim();
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "abcd");
        APSARA_TEST_EQUAL(buffer.GetTimestamp(10), 10);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(13), 10);
        buffer.RemovePrefix(2);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(0), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(5), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(11), -1);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(12), 10);
        APSARA_TEST_EQUAL(buffer.GetTimestamp(13), 10);
    }

    void TestLargeGap() {
        Buffer buffer(128, 32, 8);
        BufferAdd(buffer, "0123", 0, 0);
        auto piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123");
        APSARA_TEST_EQUAL(buffer.mPosition, 0);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 4);

        BufferAdd(buffer, "4567", 32, 10);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123");

        BufferAdd(buffer, "abcd", 100, 20);
        APSARA_TEST_EQUAL(buffer.mPosition, 92);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 12);

        BufferAdd(buffer, "test", 68, 18);
        APSARA_TEST_EQUAL(buffer.mPosition, 92);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 12);

        BufferAdd(buffer, "allow", 92, 19);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "allow");
        APSARA_TEST_EQUAL(buffer.mPosition, 92);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 12);
    }

    void TestSizeAndGetPos() {
        Buffer buffer(15, 15, 15);

        APSARA_TEST_EQUAL(buffer.mPosition, 0);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 0);
        BufferAdd(buffer, "0123", 0, 0);
        auto piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "0123");
        APSARA_TEST_EQUAL(buffer.mPosition, 0);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 4);

        BufferAdd(buffer, "abcdefghijklmno", 20, 20);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "abcdefghijklmno");
        APSARA_TEST_EQUAL(buffer.mPosition, 20);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 15);

        buffer.RemovePrefix(1);
        APSARA_TEST_EQUAL(buffer.mPosition, 21);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 14);

        buffer.RemovePrefix(14);
        APSARA_TEST_EQUAL(buffer.mPosition, 35);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 0);

        BufferAdd(buffer, "abcdefghijklmno", 120, 120);
        piece = buffer.Head();
        APSARA_TEST_EQUAL(std::string(piece.data(), piece.size()), "abcdefghijklmno");
        APSARA_TEST_EQUAL(buffer.mPosition, 120);
        APSARA_TEST_EQUAL(buffer.mBuffer.size(), 15);
    }

    void TestSizeAndGetPos1() {
        std::string ss;
        ss.resize(10);
        std::cout << ss.data() << std::endl;
        ss.resize(100);
        std::cout << ss.data() << std::endl;
        ss.resize(1000);
        std::cout << ss.data() << std::endl;
        ss.resize(100);
        std::cout << ss.data() << std::endl;
        ss.resize(10);
        std::cout << ss.data() << std::endl;
    }
};
APSARA_UNIT_TEST_CASE(ContinueBufferTest, TestAdd, 0)
APSARA_UNIT_TEST_CASE(ContinueBufferTest, TestRemovePrefixAndTrim, 0)
APSARA_UNIT_TEST_CASE(ContinueBufferTest, TestTimestamp, 0)
APSARA_UNIT_TEST_CASE(ContinueBufferTest, TestTimestampGap, 0)
APSARA_UNIT_TEST_CASE(ContinueBufferTest, TestSizeAndGetPos, 0)
APSARA_UNIT_TEST_CASE(ContinueBufferTest, TestLargeGap, 0)
APSARA_UNIT_TEST_CASE(ContinueBufferTest, TestSizeAndGetPos1, 0)

} // namespace logtail


int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
