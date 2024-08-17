#include "stringutil.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <sstream>

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

	size_t GetDisplayWidth(const std::wstring& rawString)
	{
		// TODO use FetchDisplayWidth

		// only work for text format
		if (rawString.empty()) return 0;

		std::wstring displayString = L"";
		bool inSequence = false;
		for (size_t i = 0; i < rawString.size(); i++) {
			if (inSequence) {
				if (rawString[i] == L'm') {
					inSequence = false;
				}
				continue;
			}
			if (rawString[i] == wchar_t(L'\x1b')) {
				i++;
				if (rawString.size() <= i) {
					break;
				}
				if (rawString[i] == L'[') {
					inSequence = true;
					continue;
				}
			}
			displayString += rawString[i];
		}
		if (displayString.empty()) {
			return 0;
		}
		std::vector<WORD> result(displayString.size(), 0);
		BOOL succeed = ::GetStringTypeW(CT_CTYPE3, displayString.data(), displayString.size(), result.data());
		if (!succeed) {
			auto err = ::GetLastError();
			return 0;
		}
		size_t width = 0;
		for (auto& each : result) {
			width += each & C3_HALFWIDTH ? 1 : 2;
		}
		return width;
	}

	Result<std::vector<uint8_t>> FetchDisplayWidth(const std::wstring& str)
	{
		if (str.empty()) return { std::vector<uint8_t>{}, std::nullopt };

		std::wstring displayString = L"";
		bool inSequence = false;
		for (size_t i = 0; i < str.size(); i++) {
			if (inSequence) {
				if (str[i] == L'm') {
					inSequence = false;
				}
				continue;
			}
			if (str[i] == wchar_t(L'\x1b')) {
				i++;
				if (str.size() <= i) {
					break;
				}
				if (str[i] == L'[') {
					inSequence = true;
					continue;
				}
			}
			displayString += str[i];
		}
		if (displayString.empty()) {
			return { std::vector<uint8_t>{}, std::nullopt };
		}

		std::vector<WORD> types(displayString.size(), 0);
		BOOL succeed = ::GetStringTypeW(CT_CTYPE3, displayString.data(), displayString.size(), types.data());
		if (!succeed) {
			return { std::nullopt, Error(::GetLastError(), L"Failed to ::GetStringType@stringutil") };
		}
		std::vector<uint8_t> result(types.size(), 0);
		for (int i = 0; i < types.size(); i++) {
			result[i] = types[i] & C3_HALFWIDTH ? 1 : 2;
		}
		return { result, std::nullopt };
	}

	std::wstring TruncateString(const std::wstring& source, size_t size)
	{
		if (GetDisplayWidth(source) <= size) {
			return source;
		}
		std::wstring result(source.begin(), source.begin() + size - 3);
		result += L"..";
		return result;
	}

	std::vector<std::wstring> Split(const std::wstring& source, wchar_t delimiter)
	{
		std::vector<std::wstring> result;
		std::wstring token;
		for (wchar_t ch : source) {
			if (ch == delimiter) {
				result.push_back(token);
				token.clear();
			}
			else {
				token += ch;
			}
		}
		result.push_back(token);
		return result;
	}

	std::wstring Join(const std::vector<std::wstring>& source, wchar_t delimiter)
	{
		std::wstring result;
		for (size_t i = 0; i < source.size(); ++i) {
			result += source[i];
			if (i < source.size() - 1) {
				result += delimiter;
			}
		}
		return result;
	}
}