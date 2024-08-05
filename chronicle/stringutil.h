#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include "result.h"
namespace StringUtil
{
	std::string Trim(const std::string& str);
	std::wstring Trim(const std::wstring& str);
	std::string ToAnsi(const std::wstring& str);
	std::wstring ToWide(const std::string& str);
	size_t GetDisplayWidth(const std::wstring& str);
	Result<std::vector<uint8_t>> FetchDisplayWidth(const std::wstring& str);
	std::wstring TruncateString(const std::wstring& source, size_t size);
};

