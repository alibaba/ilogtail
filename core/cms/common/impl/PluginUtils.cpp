// #ifndef WIN32
#include "common/PluginUtils.h"

#ifdef WIN32
#include <io.h>  // access
#define access _access
#else
//#include <sys/stat.h>
#include <unistd.h>  // access
//#include <dirent.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/core/demangle.hpp>

#include "common/JsonValueEx.h"
#include "common/Logger.h"
#include "common/ScopeGuard.h"
#include "common/FilePathUtils.h"

using namespace std;

namespace plugin
{
	string PluginUtils::getLastNoneEmptyLineFromFile(const std::string &filePath)
	{
		ifstream file;
		file.open(filePath);
		if (!file.is_open()) {
			return {};
		}
		defer(file.close());

		file.seekg(-1, ios_base::end);
		bool keepLooping = true;
		bool findUsefulCharacter = false;

		while (keepLooping)
		{
			char ch;
			file.get(ch);
			if ((int)file.tellg() <= 1)
			{
				file.seekg(0);
				keepLooping = false;
			}
			else if (ch != '\n' && ch != '\r')
			{
				findUsefulCharacter = true;
				file.seekg(-2, ios_base::cur);
			}
			else if (ch == '\n' && findUsefulCharacter)
			{
				keepLooping = false;
			}
			else
			{
				file.seekg(-2, ios_base::cur);
			}
		}
		string lineContent;
		std::getline(file, lineContent);
		return lineContent;
	}

	template<typename T>
	int GetFromString(const std::string &line, std::vector<T> &output, const std::function<bool(const T &)> &check) {
		std::stringstream ss(line);

		bool fail = false;
		try {
			while (ss.good())
			{
				T key;
				ss >> key;
				// �˴�ʹ��fail���жϽ��������Ƿ��쳣������ʹ��good, ����ԭ����μ�: https://cplusplus.com/reference/ios/ios/good/
				fail = ss.fail() || ss.bad();
				if (!fail && check(key)) {
					output.push_back(key);
				}
			}
		}
		catch (const std::exception &ex) {
			fail = true;
			LogError("GetVectorFromString<{}>({}), at pos: {}, error: {}",
				boost::core::demangle(typeid(T).name()), line, static_cast<int>(ss.tellg()), ex.what());
		}

		return fail? -1: static_cast<int>(output.size());
	}
	int PluginUtils::getKeys(const string &line, vector<string> &keyVector)
	{
		return GetFromString<std::string>(line, keyVector, [](const std::string &s) {return !s.empty(); });
	}
	int PluginUtils::getValues(const string &line, vector<long> &valueVector)
	{
		return GetFromString<long>(line, valueVector, [](const long &) { return true; });
	}

	int PluginUtils::GetSubDirs(const std::string &dirPath, std::vector<std::string> &subDirs)
	{
		for (const auto &entry : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied)) {
			if (fs::is_directory(entry) && !entry.path().filename_is_dot() && !entry.path().filename_is_dot_dot()) {
				std::string fullPath = entry.path().string();
				// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/access-waccess?view=msvc-170
				const int READ_ONLY = 4;
				if (access(entry.path().string().c_str(), READ_ONLY) == 0) {
					subDirs.push_back(entry.path().filename().string());
				}
			}
		}
		return static_cast<int>(subDirs.size());

		//DIR *dir;
		//struct dirent *ptr;
		//struct stat stStatBuf{};
		//if ((dir = opendir(dirPath.c_str())) != nullptr)
		//{
		//    while ((ptr = readdir(dir)) != nullptr)
		//    {
		//        if ((std::string(ptr->d_name) == ".") || (std::string(ptr->d_name) == ".."))
		//        {
		//            continue;
		//        }
		//        std::string name = dirPath+"/"+std::string(ptr->d_name);
		//        if (stat(name.c_str(), &stStatBuf) == -1 || access(name.c_str(), 4) != 0)
		//        {
		//            continue;
		//        }
		//        if (!S_ISDIR(stStatBuf.st_mode))
		//        {
		//            continue;
		//        }
		//        subDirs.emplace_back(ptr->d_name);
		//    }
		//    closedir(dir); 
		//}
		//return static_cast<int>(subDirs.size());
	}

	template<typename T>
	T GetFromFile(const std::string &file, const T &def) {
		std::ifstream fin;
		fin.open(file);
		if (fin.fail() || fin.bad()) {
			LogError("GetFromFile<{}>({}, default:{}), error: file not exist or has no permission",
				boost::core::demangle(typeid(T).name()), file, def);
			return def;
		}
		defer(fin.close()); // ��ʵ����ʱ�������ر�

		T value;
		try {
			fin >> value;
		}
		catch (const std::exception &ex) {
			LogError("GetFromFile<{}>({}, default:{}), error: {}", boost::core::demangle(typeid(T).name()), file, def, ex.what());
			value = def;
		}

		return !fin.fail() && !fin.bad() ? value: def;
	}

	uint64_t PluginUtils::GetNumberFromFile(const std::string &file)
	{
		return GetFromFile<uint64_t>(file, 0);
	}
	int64_t PluginUtils::GetSignedNumberFromFile(const std::string &file)
	{
		return GetFromFile<int64_t>(file, 0);
	}

	void PluginUtils::GetConfig(const json::Object &jsonValue, const std::string &key,
		std::vector<std::string> &valueVector) {
		jsonValue.getArray(key).toStringArray(valueVector);
		// Json::Value keyValue = jsonValue.get(key, Json::Value::null);
		// if (!keyValue.isNull() && keyValue.isArray()) {
		//     for (int i = 0; i < keyValue.size(); i++) {
		//         Json::Value elem = keyValue.get(i, Json::Value::null);
		//         if (elem.isString()) {
		//             string value = elem.asString();
		//             valueVector.push_back(value);
		//         }
		//     }
		// }
	}

} // namespace module
// #endif