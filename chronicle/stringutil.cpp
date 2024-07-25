#include "stringutil.h"
#include <Windows.h>
#include <string>
#include <vector>

namespace StringUtil
{

    std::wstring Trim(const std::wstring& str)
    {
		size_t first = str.find_first_not_of(L' ');
		if (first == std::wstring::npos)
			return L"";
		size_t last = str.find_last_not_of(L' ');
		return str.substr(first, (last - first + 1));
    }

	std::string Trim(const std::string& str)
	{
		size_t first = str.find_first_not_of(' ');
		if (first == std::string::npos)
			return "";
		size_t last = str.find_last_not_of(' ');
		return str.substr(first, (last - first + 1));
	}

	std::string ToAnsi(const std::wstring& wstr) {
		int size_needed = ::WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), nullptr, 0, nullptr, nullptr);
		std::string strTo(size_needed, 0);
		::WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), strTo.data(), size_needed, nullptr, nullptr);
		return strTo;
	}

	std::wstring ToWide(const std::string& str) {
		int size_needed = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), nullptr, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), wstrTo.data(), size_needed);
		return wstrTo;
	}

	size_t GetDisplayWidth(const std::wstring& str)
	{
		if (str.empty()) return 0;

		std::vector<WORD> result(str.size(), 0);
		BOOL succeed = ::GetStringTypeW(CT_CTYPE3, str.data(), str.size(), result.data());
		if (!succeed) {
			return -1;
		}
		size_t width = 0;
		for (auto& each : result) {
			width += each & C3_HALFWIDTH ? 1 : 2;
		}
		return width;
	}
}