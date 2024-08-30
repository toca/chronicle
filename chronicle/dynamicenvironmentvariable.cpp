#include "dynamicenvironmentvariable.h"
#include <bcrypt.h>
#include <ctime>
#include <chrono>
#include <sstream>
#pragma comment(lib, "bcrypt")
#include "errorlevel.h"

namespace Command
{
	std::wstring GetDate();
	std::wstring GetTime();
	std::wstring GetCd();
	std::wstring GetRand();
	std::wstring GetCmdLine();

	
	Result<std::wstring> ExpandDynamicEnv(const std::wstring& name)
	{
		if (_wcsicmp(name.c_str(), L"errorlevel") == 0) {
			return { GetErrorLevel(), std::nullopt };
		}
		else if (_wcsicmp(name.c_str(), L"date") == 0) {
			return { GetDate(), std::nullopt };
		}
		else if (_wcsicmp(name.c_str(), L"time") == 0) {
			return { GetTime(), std::nullopt };
		}
		else if (_wcsicmp(name.c_str(), L"cd") == 0) {
			return { GetCd(), std::nullopt };
		}
		else if (_wcsicmp(name.c_str(), L"rand") == 0) {
			return { GetRand(), std::nullopt };
		}
		else if (_wcsicmp(name.c_str(), L"cmdcmdline") == 0) {
			return { GetCmdLine(), std::nullopt };
		}
		else {
			return { std::nullopt, Error(ERROR_ENVVAR_NOT_FOUND, L"Dynamic Environment Variable was not found") };
			//return { L"%" + name + L"%", std::nullopt };
		}
	}

	std::wstring GetDate()
	{
		auto now = std::chrono::system_clock::now();
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
		std::tm now_tm{};
		::localtime_s(&now_tm, &now_time_t);
		std::wstringstream wss;
		wss.imbue(std::locale(""));
		wss << std::put_time(&now_tm, L"%x");

		return wss.str();
	}

	std::wstring GetTime()
	{
		auto now = std::chrono::system_clock::now();
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
		std::tm now_tm{};
		::localtime_s(&now_tm, &now_time_t);
		std::wstringstream wss;
		wss.imbue(std::locale(""));
		wss << std::put_time(&now_tm, L"%H:%M:%S");

		// Milli sec
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		wss << L'.' << std::setw(3) << std::setfill(L'0') << now_ms.count();

		return wss.str();
	}

	std::wstring GetCd()
	{
		wchar_t currentDirectory[MAX_PATH];
		::GetCurrentDirectoryW(MAX_PATH, currentDirectory);
		return currentDirectory;
	}

	std::wstring GetRand()
	{
		NTSTATUS status{};
		ULONG random_number{};

		status = ::BCryptGenRandom(NULL, reinterpret_cast<PUCHAR>(&random_number), sizeof(random_number), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		if (!BCRYPT_SUCCESS(status)) {
			// FIXME
			return L"";
		}

		auto rand = static_cast<int>((random_number / static_cast<double>(ULONG_MAX + 1.0)) * (RAND_MAX + 1));
		return std::to_wstring(rand);
	}

	std::wstring GetCmdLine()
	{
		LPWSTR cmdLine = ::GetCommandLineW();
		return std::wstring(cmdLine);
	}
}