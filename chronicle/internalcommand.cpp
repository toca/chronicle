#include "internalcommand.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>

#include "result.h"
#include "stringutil.h"


// TODO setlocal, endlocal
// 
namespace InternalCommand
{
	// define internal functions --------------------------
	void SaveEachDrivePath(const char driveLetter);
	std::string LoadEachDrivePath(const char driveLetter);
	/* Implementing redirection as it cannot be done with the system() function */
	Result<DWORD> System(const std::string& command, HANDLE out);

	std::vector<std::string> directoryStack{};
	std::vector<std::string> paths{ "A:\\", "B:\\", "C:\\", "D:\\", "E:\\", "F:\\", "G:\\", "H:\\", "I:\\", "J:\\", "K:\\", "L:\\", "M:\\", "N:\\",
		"O:\\", "P:\\", "Q:\\", "R:\\", "S:\\", "T:\\", "U:\\", "V:\\", "W:\\", "X:\\", "Y:\\" , "Z:\\" };
	// ----------------------------------------------------


	Result<DWORD> Dir(const std::string& param, HANDLE out)
	{
		auto [code, err] = System("dir " + param, out);
		if (err) return { std::nullopt, err };
		return { *code, std::nullopt };
	}


	Result<DWORD> Cd(const std::string& param, HANDLE out)
	{
		if (param == "/?") {
			auto [code, err] = System("cd /?", out);
			if (err) return { std::nullopt, err };
			return { *code, std::nullopt };
		}
		if (param.empty()) {
			auto [code, err] = System("cd", out);
			if (err) return { std::nullopt, err };
			return { *code, std::nullopt };
		}

		std::string option;
		std::string drive;
		std::string path;
		enum Mode {
			OPT,
			DRIVE,
			PATH,
			FINISH
		};
		Mode mode = OPT;

		for (size_t i = 0; i < param.size();) {
			char c = param[i];
			switch (mode) {
			case OPT:
				if (c == ' ') {
					i++;
					continue;
				}
				if (c == '/') {
					if (param[i + 1] == 'D' || param[i + 1] == 'd') {
						option = "/D";
						i += 2;
					}
					else if (param[i + 1] == '?') {
						option = "/?";
						i += 2;
					}
				}
				mode = DRIVE;
				break;
			case DRIVE:
				if (c == ' ') {
					i++;
					continue;
				}
				if (isalpha(c) && param[i + 1] == ':') {
					drive = std::string({ char(std::toupper(c)), ':' });
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


		char currentDirectory[MAX_PATH];
		::GetCurrentDirectoryA(MAX_PATH, currentDirectory);
		bool otherDrive = drive.empty() ? false : strncmp(currentDirectory, drive.c_str(), 2) != 0;
		bool driveOpt = option == "/D";


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
			::SetCurrentDirectoryA((drive + path).c_str());
			DWORD err = ::GetLastError();
			system(("cd " + drive + path).c_str());
			return { err, std::nullopt };
		}
		else {
			if (otherDrive && driveOpt) {
				// move other drive
				SaveEachDrivePath(currentDirectory[0]);
				std::string dist = LoadEachDrivePath(drive.at(0));
				::SetCurrentDirectoryA(dist.c_str());
				DWORD err = ::GetLastError();
				system(("cd /D " + dist).c_str());
				return { err, std::nullopt };
			}

			// only show currend saved drive
			std::string dist = LoadEachDrivePath(drive.at(0));
			printf("%s\n", dist.c_str());
			return { 0, std::nullopt };
		}
	}


	Result<DWORD> Pushd(const std::string& param, HANDLE out)
	{
		if (param == "/?") {
			auto [code, err] = System("pushd /?", out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		if (param.size()) {
			char buf[MAX_PATH];
			::GetCurrentDirectoryA(MAX_PATH, buf);
			if (::SetCurrentDirectoryA(StringUtil::Trim(param).c_str())) {
				directoryStack.push_back(buf);
			}
			else {
				return { ::GetLastError(), std::nullopt };
			}
		}
		else { // show stack
			for (auto it = directoryStack.rbegin(); it != directoryStack.rend(); it++) {
				printf("* \x1b[97m%s\x1b[0m\n", it->c_str());
			}
		}
		return { 0, std::nullopt };
	}


	Result<DWORD> Popd(const std::string& param, HANDLE out)
	{
		if (param == "/?") {
			auto [code, err] = System("pushd /?", out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		if (directoryStack.size()) {
			std::string dir = directoryStack.back();
			if (::SetCurrentDirectoryA(dir.c_str())) {
				directoryStack.pop_back();
			}
			else {
				return { ::GetLastError(), std::nullopt };
			}
		}
		return { 0, std::nullopt };
	}


	Result<DWORD> Set(const std::string& param, HANDLE out)
	{
		if (param == "/?") {
			auto [code, err] = System("set /?", out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		// /A = argebric?    /P = prompt
		if (param.starts_with("/A") || param.starts_with("/a")) {
			// TODO set value
			auto [code, err] = System(("set " + param).c_str(), out);
			if (err) return { std::nullopt, err };
			return { *code ,std::nullopt };
		}
		else if (param.starts_with("/P") || param.starts_with("/p")) {
			size_t pos = param.find('=');
			if (pos == std::string::npos) {
				// invalid arguments
				auto [code, err] = System(("set " + param).c_str(), out);
				if (err) return { std::nullopt, err };
				return { *code ,std::nullopt };
			}
			std::string name = param.substr(0, pos);
			std::string prompt = param.substr(pos + 1);
			printf("%s", prompt.c_str());
			std::string input;
			std::getline(std::cin, input);
			if (!::SetEnvironmentVariableA(name.c_str(), input.c_str())) {
				return { std::nullopt, Error(::GetLastError(), "Failed to ::SetEnvironmentVariable Set@internalcommand.cpp") };
			}
			return { 0, std::nullopt };
		}
		else {
			size_t pos = param.find('=');
			if (pos == std::string::npos) {
				// show env
				return System(("set " + param).c_str(), out);
			}
			// set env
			std::string name = param.substr(0, pos);
			std::string value = param.substr(pos + 1);
			if (!::SetEnvironmentVariableA(name.c_str(), value.c_str())) {
				return { std::nullopt, Error(::GetLastError(), "Failed to ::SetEnvironmentVariable Set@internalcommand.cpp") };
			}
			return { 0, std::nullopt };
		}
	}


	DWORD Echo(const std::string& param, HANDLE out)
	{
		DWORD written = 0;
		if(::WriteFile(out, param.data(), param.size(), &written, nullptr)) {
			if (param.size() != size_t(written)) {
				return ::GetLastError();
			}
			::WriteFile(out, "\n", 1, &written, nullptr);
			return 0;
		}
		else {
			return ::GetLastError();
		}
	}


	// internal functions ---------------------------------
	bool IsDriveLetter(const std::string& str)
	{
		// expect like C:
		if (str.length() != 2) {
			return false;
		}
		// first character must be alpha
		if (!std::isalpha(str[0])) {
			return false;
		}
		// second character must be :
		if (str[1] != ':') {
			return false;
		}
		return true;
	}


	void SaveEachDrivePath(const char driveLetter)
	{
		size_t index = std::tolower(driveLetter) - 'a';
		char buf[MAX_PATH];
		::GetCurrentDirectoryA(MAX_PATH, buf);
		paths[index] = std::string(buf);
	}


	std::string LoadEachDrivePath(const char driveLetter)
	{
		size_t index = std::tolower(driveLetter) - 'a';
		return paths.at(index);
	}


	Result<DWORD> System(const std::string& command, HANDLE out)
	{
		// launch cmd.exe

		PROCESS_INFORMATION processInfo{};

		STARTUPINFOA startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
		startupInfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
		startupInfo.hStdOutput = out;

		// TODO escape?
		std::string shellCommand = "cmd /Q /C " + command;
		std::vector<char> commandLine(shellCommand.data(), shellCommand.data() + shellCommand.size() + 1);
		// launch
		auto res = ::CreateProcessA(
			nullptr,
			commandLine.data(),
			nullptr,
			nullptr,
			TRUE,
			//CREATE_NEW_CONSOLE,
			0, // if 0 Ctrl + C send to each process
			nullptr,
			nullptr,
			&startupInfo,
			&processInfo
		);
		if (!res) {
			return { std::nullopt, Error(::GetLastError(), "Failed to ::CreateProcess") };
		}

		// wait for process exit
		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		DWORD exitCode = 0;
		if (!::GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, "Failed to ::GetExitCodeProcess System@internalcommand.cpp") };
		}
		::CloseHandle(processInfo.hThread);
		::CloseHandle(processInfo.hProcess);
		return { exitCode, std::nullopt };
		
	}
}