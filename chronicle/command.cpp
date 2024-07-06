#include "command.h"
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <cctype>

#include "parse.h"
#include "error.h"
#include "result.h"

// global
extern DWORD ERRORLEVEL;

namespace Command
{
	// enum
	enum class Type
	{
		EXTERNAL,
		EMPTY,
		CHANGE_DRIVE,
		CD,
		PUSHD,
		POPD,
		SET
	};


	// internal definition ----

	struct Command
	{
		// "C:Program Files\Foo\bar\pg.exe -opt1 -opt2"
		const std::string operation;  // C:Program Files\Foo\bar\pg.exe
		const std::string parameter;  // -opt1 -opt2
	};
	//Command Parse(const std::string& input);
	Type ClassifyCommand(const std::string& command);
	std::string Trim(const std::string& str);
	bool IsDriveLetter(const std::string& str);
	void SaveEachDrivePath(const char driveLetter);
	std::string LoadEachDrivePath(const char driveLetter);

	// command
	DWORD ExecuteCommand(const std::string& command, const std::string& arguments);
	DWORD Cd(const std::string& param);
	DWORD Pushd(const std::string& param);
	DWORD Popd(const std::string& param);
	DWORD Set(const std::string& param);

	// functions ----


	std::vector<std::string> directoryStack{};

	OptionalError Execute(const std::string& input)
	{

		auto [nodes, err] = Parse(input);
		if (err) {
			//fprintf(stderr, "The syntax of the command is incorrect.");
			return Error(err->code, "The syntax of the command is incorrect.");
		}

		// current process
		// process list?
		// current 1
		// current 2
		// >, >>, &, &&, ||, | 
		// 2>&1
		for (size_t i = 0; i < nodes->size(); i++)
		{
			switch (nodes->at(i).type)
			{
			case NodeType::End:
				return std::nullopt;
			case NodeType::Command:
				if (nodes->at(i + 1).type == NodeType::Redirect && nodes->at(i + 2).type == NodeType::File) {

				}
				else {
					ExecuteCommand(nodes->at(i).command, nodes->at(i).arguments);
				}
			}

			/*Type type = ClassifyCommand(nodes->at(i).command);
			DWORD result = 0;
			if (type == Type::EXTERNAL) {
				result = system(input.c_str());
				::SetEnvironmentVariableA("ERRORLEVEL", std::to_string(result).c_str());
			}

			switch (type)
			{
			case Type::CHANGE_DRIVE:
				ERRORLEVEL = Cd("/D " + nodes->at(i).command);
				break;
			case Type::CD:
				ERRORLEVEL = Cd(nodes->at(i).arguments);
				break;
			case Type::PUSHD:
				ERRORLEVEL = Pushd(nodes->at(i).arguments);
				break;
			case Type::POPD:
				ERRORLEVEL = Popd(nodes->at(i).arguments);
				break;
			case Type::SET:
				ERRORLEVEL = Set(nodes->at(i).arguments);
				break;
			case Type::EXTERNAL:
				break;
			default:
				break;
			}*/
		}
		return std::nullopt;

		// TODO parse input e.g. "echo one & echo two"
		// &  &&  ||
		// TODO redirect? > pipe? |
		// NUL => HANDLE hFile = CreateFile("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		// At first impl pipe |
		// difference of beween | and > are only handle?
		// parse: command <parameter> < '|' '>' '<' '>>' '&' '&&' '||' > command ...

		//Command command = Parse(input);
		//Type type = ClassifyCommand(command.operation);
		//
		//DWORD result = 0;
		//if (type == Type::EXTERNAL) {
		//	result = system(input.c_str());
		//	::SetEnvironmentVariableA("ERRORLEVEL", std::to_string(result).c_str());
		//}
		//
		//switch (ClassifyCommand(command.operation))
		//{
		//case Type::CHANGE_DRIVE:
		//	ERRORLEVEL = Cd("/D " + command.operation);
		//	break;
		//case Type::CD:
		//	ERRORLEVEL = Cd(command.parameter);
		//	break;
		//case Type::PUSHD:
		//	ERRORLEVEL = Pushd(command.parameter);
		//	break;
		//case Type::POPD:
		//	ERRORLEVEL = Popd(command.parameter);
		//	break;
		//case Type::SET:
		//	ERRORLEVEL = Set(command.parameter);
		//	break;
		//case Type::EXTERNAL:
		//	break;
		//default:
		//	break;
		//}

		//puts("");
		//return result;
	}

	// command ---- -----

	DWORD ExecuteCommand(const std::string& command, const std::string& arguments)
	{
		Type type = ClassifyCommand(command);
		DWORD result = 0;
		if (type == Type::EXTERNAL) {
			result = system((command + " " + arguments).c_str());
			::SetEnvironmentVariableA("ERRORLEVEL", std::to_string(result).c_str());
		}

		switch (type)
		{
		case Type::CHANGE_DRIVE:
			ERRORLEVEL = Cd("/D " + command);
			break;
		case Type::CD:
			ERRORLEVEL = Cd(arguments);
			break;
		case Type::PUSHD:
			ERRORLEVEL = Pushd(arguments);
			break;
		case Type::POPD:
			ERRORLEVEL = Popd(arguments);
			break;
		case Type::SET:
			ERRORLEVEL = Set(arguments);
			break;
		case Type::EXTERNAL:
			break;
		default:
			break;
		}
		return result;
	}
	
	DWORD Cd(const std::string& param)
	{
		if (param == "/?") {
			return system("cd /?");
		}
		if (param.empty()) {
			return system("cd");
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
				return 0;
			}
			
			SaveEachDrivePath(currentDirectory[0]);
			::SetCurrentDirectoryA((drive + path).c_str());
			DWORD err = ::GetLastError();
			system(("cd " + drive + path).c_str());
			return err;
		}
		else {
			if (otherDrive && driveOpt) {
				// move other drive
				SaveEachDrivePath(currentDirectory[0]);
				std::string dist = LoadEachDrivePath(drive.at(0));
				::SetCurrentDirectoryA(dist.c_str());
				DWORD err = ::GetLastError();
				system(("cd /D " + dist).c_str());
				return err;
			}

			// only show currend saved drive
			std::string dist = LoadEachDrivePath(drive.at(0));
			printf("%s\n", dist.c_str());
			return 0;
		}
	}


	DWORD Pushd(const std::string& param)
	{
		if (param == "/?") {
			return system("pushd /?");
		}
		if (param.size()) {
			char buf[MAX_PATH];
			::GetCurrentDirectoryA(MAX_PATH, buf);
			if (::SetCurrentDirectoryA(Trim(param).c_str())) {
				directoryStack.push_back(buf);
			} else {
				return ::GetLastError();
			}
		}
		else { // show stack
			for (auto it = directoryStack.rbegin(); it != directoryStack.rend(); it++) {
				printf("* \x1b[97m%s\x1b[0m\n", it->c_str());
			}
		}
		return 0;
	}


	DWORD Popd(const std::string& param)
	{
		if (param == "/?") {
			return system("pushd /?");
		}
		if (directoryStack.size()){
			std::string dir = directoryStack.back();
			if (::SetCurrentDirectoryA(dir.c_str())) {
				directoryStack.pop_back();
			}
			else {
				return ::GetLastError();
			}
		}
		return 0;
	}


	DWORD Set(const std::string& param)
	{
		if (param == "/?") {
			return system("set /?");
		}
		// /A = argebric?    /P = prompt
		if (param.starts_with("/A") || param.starts_with("/a")) {
			return system(("set " + param).c_str());
		}
		else if (param.starts_with("/P") || param.starts_with("/p")) {
			size_t pos = param.find('=');
			if (pos == std::string::npos) {
				// invalid arguments
				return system(("set " + param).c_str());
			}
			std::string name = param.substr(0, pos);
			std::string prompt = param.substr(pos + 1);
			printf("%s", prompt.c_str());
			std::string input;
			std::getline(std::cin, input);
			if (!::SetEnvironmentVariableA(name.c_str(), input.c_str())) {
				return ::GetLastError();
			}
			return 0;
		}
		else {
			size_t pos = param.find('=');
			if (pos == std::string::npos) {
				// show env
				return system(("set " + param).c_str());
			}
			// set env
			std::string name = param.substr(0, pos);
			std::string value = param.substr(pos + 1);
			if (!::SetEnvironmentVariableA(name.c_str(), value.c_str())) {
				return ::GetLastError();
			}
			return 0;
		}
	}

	// internal functions ---- ----


	//Command Parse(const std::string& input)
	//{
	//	bool quote = false;
	//	std::string operation;
	//	size_t i = 0;
	//	for (; i < input.size(); i++) {
	//		if (operation.empty() && input[i] == ' ') {
	//			continue;
	//		}
	//		//else if (input[i] == '\\') {
	//		//	i++;
	//		//}
	//		else if (input[i] == '\"') {
	//			quote = !quote;
	//		}
	//		else if (input[i] == ' ' && !quote) {
	//			i++;
	//			break;
	//		}
	//		operation += input[i];
	//	}
	//	return Command{ operation, Trim(input.substr(i))};
	//}


	Type ClassifyCommand(const std::string& command)
	{
		if (command.empty()) {
			return Type::EMPTY;
		}
		else if(_stricmp(command.c_str(), "cd") == 0) {
			return Type::CD;
		}
		else if (_stricmp(command.c_str(), "pushd") == 0) {
			return Type::PUSHD;
		}
		else if (_stricmp(command.c_str(), "popd") == 0) {
			return Type::POPD;
		}
		else if (_stricmp(command.c_str(), "set") == 0) {
			return Type::SET;
		}
		else if (IsDriveLetter(command)) {
			return Type::CHANGE_DRIVE;
		}
		else {
			return Type::EXTERNAL;
		}
	}


	std::string Trim(const std::string& str) {
		size_t first = str.find_first_not_of(' ');
		if (first == std::string::npos)
			return "";
		size_t last = str.find_last_not_of(' ');
		return str.substr(first, (last - first + 1));
	}


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


	std::vector<std::string> paths{ "A:\\", "B:\\", "C:\\", "D:\\", "E:\\", "F:\\", "G:\\", "H:\\", "I:\\", "J:\\", "K:\\", "L:\\", "M:\\", "N:\\",
		"O:\\", "P:\\", "Q:\\", "R:\\", "S:\\", "T:\\", "U:\\", "V:\\", "W:\\", "X:\\", "Y:\\" , "Z:\\" };

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
}


