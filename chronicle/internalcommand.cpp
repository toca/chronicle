#include "internalcommand.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <cwctype>

#include "consoleutil.h"
#include "error.h"
#include "result.h"
#include "stringutil.h"


// TODO setlocal, endlocal
namespace InternalCommand
{
	// define internal functions --------------------------
	void SaveEachDrivePath(const wchar_t driveLetter);
	std::wstring LoadEachDrivePath(const wchar_t driveLetter);
	/* Implementing redirection as it cannot be done with the system() function */
	Result<DWORD> System(const std::wstring& command, HANDLE out);

	std::vector<std::wstring> directoryStack{};
	std::vector<std::wstring> paths{ L"A:\\", L"B:\\", L"C:\\", L"D:\\", L"E:\\", L"F:\\", L"G:\\", L"H:\\", L"I:\\", L"J:\\", L"K:\\", L"L:\\", L"M:\\", L"N:\\",
		L"O:\\", L"P:\\", L"Q:\\", L"R:\\", L"S:\\", L"T:\\", L"U:\\", L"V:\\", L"W:\\", L"X:\\", L"Y:\\" , L"Z:\\" };
	// ----------------------------------------------------


	Result<DWORD> Dir(const std::wstring& param, HANDLE out)
	{
		auto [code, err] = System(L"dir " + param, out);
		if (err) return { std::nullopt, err };
		return { *code, std::nullopt };
	}


	Result<DWORD> Cd(const std::wstring& param, HANDLE out)
	{
		if (param == L"/?") {
			auto [code, err] = System(L"cd /?", out);
			if (err) return { std::nullopt, err };
			return { *code, std::nullopt };
		}
		if (param.empty()) {
			auto [code, err] = System(L"cd", out);
			if (err) return { std::nullopt, err };
			return { *code, std::nullopt };
		}

		std::wstring option;
		std::wstring drive;
		std::wstring path;
		enum Mode {
			OPT,
			DRIVE,
			PATH,
			FINISH
		};
		Mode mode = OPT;

		for (size_t i = 0; i < param.size();) {
			wchar_t c = param[i];
			switch (mode) {
			case OPT:
				if (c == L' ') {
					i++;
					continue;
				}
				if (c == L'/') {
					if (param[i + 1] == L'D' || param[i + 1] == L'd') {
						option = L"/D";
						i += 2;
					}
					else if (param[i + 1] == L'?') {
						option = L"/?";
						i += 2;
					}
				}
				mode = DRIVE;
				break;
			case DRIVE:
				if (c == L' ') {
					i++;
					continue;
				}
				if (isalpha(c) && param[i + 1] == L':') {
					drive = std::wstring({ wchar_t(std::towupper(c)), L':' });
					i += 2;
				}
				mode = PATH;
				break;
			case PATH:
				path += c;
				i++;
				break;
			case FINISH:
				i++;
				break;
			}
		}


		wchar_t currentDirectory[MAX_PATH];
		::GetCurrentDirectory(MAX_PATH, currentDirectory);
		bool otherDrive = drive.empty() ? false : wcsncmp(currentDirectory, drive.c_str(), 2) != 0;
		bool driveOpt = option == L"/D";


		// TODO if drive not found when cd D:
		// TODO Fix first location in the list. It is wrong.
		/*
		* - Path specified ==============================
		*                      -cross drive-
		*        +------------+------------+-----------+
		*        |            |    True    |   False   |
		*        +------------+------------+-----------+
		*  -/D-  |    True    |      1     |     2     |
		*        +------------+------------+-----------+
		*        |    False   |      3     |     4     |
		*        +------------+------------+-----------+
		*
		* - Path not specified ==========================
		*                      -cross drive-
		*        +------------+------------+-----------+
		*        |            |    True    |   False   |
		*        +------------+------------+-----------+
		*  -/D-  |    True    |      5     |     6     |
		*        +------------+------------+-----------+
		*        |    False   |      7     |     8     |
		*        +------------+------------+-----------+
		*
		*  1. Move to other drive and path.
		*  2. Move to other drive and path.
		*  3. Do nothing with no error.
		*  4. Move to the same drive.
		*  5. Move to other drive. Use saved path.
		*  6. Move to other drive. Use saved path.
		*  7. Show saved path.
		*  8. Show saved path.
		*/
		if (path.size()) {
			// cd Z:\foobar
			if (otherDrive && !driveOpt) {
				return { 0, std::nullopt };
			}

			SaveEachDrivePath(currentDirectory[0]);
			System(L"cd " + drive + path, out);

			DWORD err = 0;
			if (!::SetCurrentDirectoryW((drive + path).c_str())) {
				err = ::GetLastError();
				return { err, std::nullopt };
			}
			return { ERROR_SUCCESS, std::nullopt };
		}
		else {
			if (otherDrive && driveOpt) {
				// move other drive
				SaveEachDrivePath(currentDirectory[0]);
				std::wstring dist = LoadEachDrivePath(drive.at(0));

				System(L"cd /D " + dist, out);
				if (!::SetCurrentDirectoryW(dist.c_str())) {
					DWORD err = ::GetLastError();
					return { err, std::nullopt };
				}
				return { ERROR_SUCCESS, std::nullopt };
			}

			// only show currend saved drive
			std::wstring dist = LoadEachDrivePath(drive.at(0));
			dist += L'\n';
			DWORD written = 0;
			std::string ansi = StringUtil::ToAnsi(dist);
			if (!::WriteFile(out, ansi.c_str(), ansi.size(), &written, nullptr)) {
				DWORD err = ::GetLastError();
				return { std::nullopt, Error(err, L"Failed to ::WriteFile Cd@internalcommand") };
			}
			return { 0, std::nullopt };
		}
	}


	Result<DWORD> Pushd(const std::wstring& param, HANDLE out)
	{
		if (param == L"/?") {
			auto [code, err] = System(L"pushd /?", out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		if (param.size()) {
			wchar_t buf[MAX_PATH];
			::GetCurrentDirectory(MAX_PATH, buf);
			if (::SetCurrentDirectoryW(StringUtil::Trim(param).c_str())) {
				directoryStack.push_back(buf);
			}
			else {
				return { ::GetLastError(), std::nullopt };
			}
		}
		else { // show stack
			for (auto it = directoryStack.rbegin(); it != directoryStack.rend(); it++) {
				// FIXME
				wprintf(L"* \x1b[97m%s\x1b[0m\n", it->c_str());
			}
		}
		return { 0, std::nullopt };
	}


	Result<DWORD> Popd(const std::wstring& param, HANDLE out)
	{
		if (param == L"/?") {
			auto [code, err] = System(L"pushd /?", out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		if (directoryStack.size()) {
			std::wstring dir = directoryStack.back();
			if (::SetCurrentDirectoryW(dir.c_str())) {
				directoryStack.pop_back();
			}
			else {
				return { ::GetLastError(), std::nullopt };
			}
		}
		return { 0, std::nullopt };
	}


	Result<DWORD> Set(const std::wstring& param, HANDLE out)
	{
		if (param == L"/?") {
			auto [code, err] = System(L"set /?", out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		// /A = argebric?    /P = prompt
		if (param.starts_with(L"/A") || param.starts_with(L"/a")) {
			// TODO set value
			auto [code, err] = System((L"set " + param).c_str(), out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		else if (param.starts_with(L"/P") || param.starts_with(L"/p")) {
			size_t pos = param.find(L'=');
			if (pos == std::string::npos) {
				// invalid arguments
				auto [code, err] = System((L"set " + param).c_str(), out);
				if (err) return { std::nullopt, err };
				return { *code ,std::nullopt };
			}
			std::wstring name = param.substr(0, pos);
			std::wstring prompt = param.substr(pos + 1);
			
			DWORD written = 0;
			::WriteConsoleW(out, prompt.c_str(), prompt.size(), &written, nullptr);

			std::wstring input;
			std::getline(std::wcin, input);
			if (!::SetEnvironmentVariableW(name.c_str(), input.c_str())) {
				return { std::nullopt, Error(::GetLastError(), L"Failed to ::SetEnvironmentVariable Set@internalcommand.cpp") };
			}
			return { 0, std::nullopt };
		}
		else {
			size_t pos = param.find(L'=');
			if (pos == std::string::npos) {
				// show env
				return System((L"set " + param).c_str(), out);
			}
			// set env
			std::wstring name = param.substr(0, pos);
			std::wstring value = param.substr(pos + 1);
			if (!::SetEnvironmentVariableW(name.c_str(), value.c_str())) {
				return { std::nullopt, Error(::GetLastError(), L"Failed to ::SetEnvironmentVariable Set@internalcommand.cpp") };
			}
			return { 0, std::nullopt };
		}
	}


	DWORD Echo(const std::wstring& param, HANDLE out)
	{
		// TODO output "ECHO is on." if empty.
		// Use System?
		DWORD written = 0;
		std::string ansi = StringUtil::ToAnsi(param);
		if(::WriteFile(out, ansi.data(), ansi.size(), &written, nullptr)) {
			if (ansi.size() != size_t(written)) {
				return ::GetLastError();
			}
			::WriteFile(out, "\n", 1, &written, nullptr);
			return 0;
		}
		else {
			return ::GetLastError();
		}
	}


	Result<DWORD> Cls(HANDLE out)
	{
		auto [info, err] = ConsoleUtil::GetConsoleScreenBufferInfo();
		if (err) {
			return { std::nullopt, err };
		}

		HANDLE stdOutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD cellCount = info->dwSize.X * info->dwSize.Y;
		DWORD count = 0;
		// Fill screen buffer as space.
		if (!::FillConsoleOutputCharacter(stdOutHandle,  L' ', cellCount, { 0, 0 }, &count)) {
			return { std::nullopt, Error(::GetLastError(), L"Failed to ::FillConsoleOutputCharacter@internalcommand") };
		}

		// Move cursor to Top, Left
		if (!::SetConsoleCursorPosition(stdOutHandle, { 0, 0 })) {
			return { std::nullopt, Error(::GetLastError(), L"Failed to ::SetConsoleCursorPosition@internalcommand") };
		}

		return  { 0, std::nullopt };
	}

	// internal functions ---------------------------------
	bool IsDriveLetter(const std::wstring& str)
	{
		// expect like C:
		if (str.length() != 2) {
			return false;
		}
		// first character must be alpha
		if (!std::iswalpha(str[0])) {
			return false;
		}
		// second character must be :
		if (str[1] != L':') {
			return false;
		}
		return true;
	}


	void SaveEachDrivePath(const wchar_t driveLetter)
	{
		size_t index = std::towlower(driveLetter) - L'a';
		wchar_t buf[MAX_PATH];
		::GetCurrentDirectoryW(MAX_PATH, buf);
		paths[index] = std::wstring(buf);
	}


	std::wstring LoadEachDrivePath(const wchar_t driveLetter)
	{
		size_t index = std::towlower(driveLetter) - L'a';
		return paths.at(index);
	}


	Result<DWORD> System(const std::wstring& command, HANDLE out)
	{
		// launch cmd.exe

		PROCESS_INFORMATION processInfo{};

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
		startupInfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
		startupInfo.hStdOutput = out;

		// TODO escape?
		std::wstring shellCommand = L"cmd /Q /C " + command;
		std::vector<wchar_t> commandLine(shellCommand.data(), shellCommand.data() + shellCommand.size() + 1);
		// launch
		auto res = ::CreateProcessW(
			nullptr,
			commandLine.data(),
			nullptr,
			nullptr,
			TRUE,
			0, // if 0 Ctrl + C send to each process
			nullptr,
			nullptr,
			&startupInfo,
			&processInfo
		);
		if (!res) {
			return { std::nullopt, Error(::GetLastError(), L"Failed to ::CreateProcess") };
		}

		// wait for process exit
		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		DWORD exitCode = 0;
		if (!::GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, L"Failed to ::GetExitCodeProcess System@internalcommand.cpp") };
		}
		::CloseHandle(processInfo.hThread);
		::CloseHandle(processInfo.hProcess);
		return { exitCode, std::nullopt };
		
	}
}