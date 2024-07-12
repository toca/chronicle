#pragma once
#include <Windows.h>
#include <string>
#include "result.h"
#include "error.h"

// enum
enum class Type
{
	EXTERNAL,
	EMPTY,
	DIR,
	CHANGE_DRIVE,
	CD,
	PUSHD,
	POPD,
	SET,
	ECHO
};

class Process
{
public:
	Process(const std::string& command, const std::string& arguments, HANDLE input, HANDLE output, HANDLE error);
	~Process();
	OptionalError Start();
	Result<DWORD> WaitForExit();

	const std::string command;
	const std::string arguments;

private:
	HANDLE input;
	HANDLE output;
	HANDLE error;
	PROCESS_INFORMATION processInfo;
	DWORD exitCode;
	Type type;

	void Create();

};

