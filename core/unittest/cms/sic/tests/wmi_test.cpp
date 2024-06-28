#include <gtest/gtest.h>
#include "sic/wmi.h"
#include "sic/system_information_collector_util.h"
#include "sic/win32_sic_utils.h"
#include "common/Common.h" // GetPid

class SicWmiTest : public testing::Test
{
protected:
	void SetUp() override
	{
		ASSERT_TRUE(sicWmi.wbem == nullptr);
	}
	void TearDown() override
	{
	}

	SicWMI sicWmi;
};

TEST_F(SicWmiTest, GetLastError) {
	sicWmi.result = S_OK;
	EXPECT_EQ(ERROR_SUCCESS, sicWmi.GetLastError());
}


TEST_F(SicWmiTest, OpenWithWbem) {
	sicWmi.wbem = (IWbemServices*)new char;
	EXPECT_TRUE(sicWmi.Open());
	EXPECT_EQ(S_OK, sicWmi.result);
	delete (char*)(sicWmi.wbem);
	sicWmi.wbem = nullptr;
}

TEST_F(SicWmiTest, GetProcessExe) {
	std::string exe;
	EXPECT_EQ(SicWMI::GetProcessExecutablePath(GetPid(), exe), 0);
	std::cout << "PID(" << GetPid() << ") " << exe << std::endl;
	EXPECT_FALSE(exe.empty());

	bstr_t args;
	EXPECT_EQ(SicWMI::GetProcessArgs(GetPid(), args), 0);
	std::string s = WcharToChar(args, CP_ACP);
	std::cout << "PID(" << GetPid() << ") " << s << std::endl;
	EXPECT_FALSE(s.empty());
}

TEST_F(SicWmiTest, EnumProcesses) {
	std::vector<WmiProcessInfo> processes;
	SicWMI sicWmi;
	ASSERT_TRUE(sicWmi.Open());
	EXPECT_TRUE(sicWmi.EnumProcesses(processes));
	EXPECT_FALSE(processes.empty());

	DWORD pid = GetPid();
	
	bool found = false;
	for (auto& it : processes) {
		if (it.pid == pid) {
			found = true;
			std::cout << "PID(" << it.pid << ") Exe: " << it.exe << ", Args: " << WcharToChar(it.args, CP_ACP) << std::endl;
			EXPECT_FALSE(it.exe.empty());
			break;
		}
	}
	EXPECT_TRUE(found);
}

TEST_F(SicWmiTest, GetDiskSerialId) {
	SicWMI sicWmi;
	ASSERT_TRUE(sicWmi.Open());
	
	std::vector<WmiPartitionInfo> disks;
	EXPECT_TRUE(sicWmi.GetDiskSerialId(disks));
	int count = 0;
	for(const auto &it: disks) {
		std::cout << "[" << (++count) << "] " << it.partition << " => " << it.sn << std::endl;
	}
}
