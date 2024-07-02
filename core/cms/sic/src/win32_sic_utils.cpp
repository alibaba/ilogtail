#include "../win32_sic_utils.h"

#include "common/Logger.h"
#include "common/Defer.h"

#include <Shlobj.h>
#pragma comment(lib, "Shell32.lib") // internet explorer 5.0 or later (Windows 2000)

#include <map>
#include <tuple>

std::string WcharToChar(const wchar_t* wideCharStr, DWORD CodePage) {
    std::string ret;
    int srcLen = wideCharStr == nullptr? 0: (int)wcslen(wideCharStr);
    if (srcLen > 0) {
        int neededSize = WideCharToMultiByte(CodePage, 0, wideCharStr, srcLen, nullptr, 0, nullptr, nullptr);
        ret = std::string((size_t)neededSize, '\0');
        WideCharToMultiByte(CodePage, 0, wideCharStr, srcLen, &ret[0], neededSize, nullptr, nullptr);
    }
    return ret;
}


#if 0
static fs::path callFunc(const char *funcName, UINT(*fun)(LPSTR lpBuffer, UINT  uSize)) {
	char path[MAX_PATH + 1] = { 0 };
	UINT pathLength = fun(path, sizeof(path));
	if (pathLength == 0) {
		LogError("{}", ErrorStr(GetLastError(), funcName, ));
	}
	return { path };
}

// GetSystemWow64Directory��SysWOW64: https://stackoverflow.com/questions/52986454/not-able-to-get-the-correct-path-using-getsystemdirectory-in-64-bit-machine
static std::map<EnumDirectoryType, std::function<fs::path()>> mapSpecialDirectory{
	{EnumDirectoryType::System32, []() { return callFunc("GetSystemDirectory", GetSystemDirectory); }},
	{EnumDirectoryType::Windows, []() {return callFunc("GetWindowsDirectory", GetWindowsDirectory); }},
};

fs::path GetSpecialDirectory(EnumDirectoryType dirType) {
	fs::path path;
	auto it = mapSpecialDirectory.find(dirType);
	if (it != mapSpecialDirectory.end()) {
		path = (it->second)();
	}
	return path;
}
#else
// SHGetFolderPathA function: https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetfolderpatha
// GetSystemWow64Directory��SysWOW64: https://stackoverflow.com/questions/52986454/not-able-to-get-the-correct-path-using-getsystemdirectory-in-64-bit-machine
// CSIDL: https://learn.microsoft.com/en-us/windows/win32/shell/csidl
// Shell and Shlwapi DLL Versions: https://learn.microsoft.com/en-us/windows/win32/shell/versions
static const auto &mapSpecialDirectory = *new std::map<EnumDirectoryType, std::tuple<int, const char *>>{
	{EnumDirectoryType::System32, std::tuple<int, const char*>{CSIDL_SYSTEM, "CSIDL_SYSTEM"} },
	{EnumDirectoryType::Windows, std::tuple<int, const char*>{CSIDL_WINDOWS, "CSIDL_WINDOWS" }},
	{EnumDirectoryType::ProgramFiles, std::tuple<int, const char*>{CSIDL_PROGRAM_FILES, "CSIDL_PROGRAM_FILES" }},
};
fs::path GetSpecialDirectory(EnumDirectoryType dirType) {
	char szPath[MAX_PATH + 1] = { 0 };
	auto it = mapSpecialDirectory.find(dirType);
	if (it != mapSpecialDirectory.end()) {
		HRESULT result = SHGetFolderPath(NULL, std::get<0>(it->second), NULL, 0, szPath);
		if (S_OK != result) {
			std::string name;
			name.append("SHGetFolderPath(")
				.append(std::get<1>(it->second))
				.append(")"); // SHGetFolderPath(CSIDL_WINDOWS)
			LogError("{}", ErrorStr(GetLastError(), name));
		}
	}

	return szPath[0] ? fs::path{ szPath } : fs::path{};
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// LastError ��GetLoastError���ص���ֵ����ʽ��Ϊ�ɶ��ַ���
std::string ErrorStr(DWORD errCode, const std::string& functionName)
{
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);
	defer(LocalFree(lpMsgBuf));

	std::ostringstream oss;
	oss << functionName << (functionName.empty() ? "" : " ")
		<< "failed with error " << errCode << ": "
		<< (LPCTSTR)(lpMsgBuf? lpMsgBuf: "");
	return oss.str();
}


DWORD LastError(const std::string &functionName, std::string* errMsg)
{
	DWORD errCode = GetLastError();
	if (errMsg != nullptr) {
		*errMsg = ErrorStr(errCode, functionName);
	}
	return errCode;
}
