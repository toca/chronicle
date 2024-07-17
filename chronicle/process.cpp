#include "process.h"
#include <string>
#include <vector>
#include "internalcommand.h"
#include "result.h"
#include "error.h"
#include "stringutil.h"



Type ClassifyCommand(const std::string& command);




Process::Process(const std::string& cmd, const std::string& arg, HANDLE in, HANDLE out, HANDLE err)
	: command(cmd)
	, arguments(arg)
	, input(in)
	, output(out)
	, error(err)
	, processInfo({})
	, exitCode(0)
{
	this->type = ClassifyCommand(this->command);
}


Process::~Process()
{
	::CloseHandle(this->processInfo.hProcess);
	::CloseHandle(this->processInfo.hThread);
}


OptionalError Process::Start()
{
	
	if (type == Type::EXTERNAL) {
		this->Create();
		return std::nullopt;
	}

	switch (type)
	{
	case Type::DIR:
	{
		auto [code, err] = InternalCommand::Dir(arguments, this->output);
		if (!err) {
			this->exitCode = *code;
		}
		else {
			return err;
		}
		break;
	}
	case Type::CHANGE_DRIVE:
	{
		auto [code, err] = InternalCommand::Cd("/D " + command, this->output);
		if (!err) {
			this->exitCode = *code;
		}
		else {
			return err;
		}
		break;
	}
	case Type::CD:
	{
		auto [code, err] = InternalCommand::Cd(arguments, this->output);
		if (!err) {
			this->exitCode = *code;
		}
		else {
			return err;
		}
		break;
	}
	case Type::PUSHD:
	{
		auto [code, err] = InternalCommand::Pushd(arguments, this->output);
		if (!err) {
			this->exitCode = *code;
		}
		else {
			return err;
		}
		break;
	}
	case Type::POPD:
	{
		auto [code, err] = InternalCommand::Popd(arguments, this->output);
		if (!err) {
			this->exitCode = *code;
		}
		else {
			return err;
		}
		break;
	}
	case Type::SET:
	{
		auto [code, err] = InternalCommand::Set(arguments, this->output);
		if (!err) {
			this->exitCode = *code;
		}
		else {
			return err;
		}
		break;
	}
	case Type::ECHO:
		this->exitCode = InternalCommand::Echo(arguments, this->output);
		break;
	case Type::EXTERNAL:
		break;
	default:
		break;
	}
	return std::nullopt;
}


Result<DWORD> Process::WaitForExit()
{
	if (this->type == Type::EXTERNAL) {
		if (this->exitCode != 0) {
			// failed to CreateProcess
			return { this->exitCode, std::nullopt };
		}
		::WaitForSingleObject(this->processInfo.hProcess, INFINITE);
		if (!::GetExitCodeProcess(this->processInfo.hProcess, &(this->exitCode))) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, "Failed to ::GetExitCodeProcess WaitForExit@process.cpp") };
		}
	}
	return { exitCode, std::nullopt };
}


void Process::Create()
{
	// launch cmd.exe
	STARTUPINFOA startupInfo{};
	startupInfo.cb = sizeof(startupInfo);

	startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	startupInfo.hStdInput = this->input;
	startupInfo.hStdOutput = this->output;
	startupInfo.hStdError = this->error;

	std::string commandAndArguments = this->command + " " + this->arguments;
	std::vector<char> commandLine(commandAndArguments.data(),commandAndArguments.data() + commandAndArguments.size() + 1);
	// launch
	auto res = ::CreateProcessA(
		nullptr,
		commandLine.data(),
		nullptr,
		nullptr,
		TRUE,
		0, // if 0 Ctrl + C send to each process
		nullptr,
		nullptr,
		&startupInfo,
		&this->processInfo
	);
	if (!res) {
		this->exitCode = ::GetLastError();
	}
}




Type ClassifyCommand(const std::string& command)
{
	if (command.empty()) {
		return Type::EMPTY;
	}
	else if (_stricmp(command.c_str(), "dir") == 0) {
		return Type::DIR;
	}
	else if (_stricmp(command.c_str(), "cd") == 0) {
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
	else if (_stricmp(command.c_str(), "echo") == 0) {
		return Type::ECHO;
	}
	else if (InternalCommand::IsDriveLetter(command)) {
		return Type::CHANGE_DRIVE;
	}
	else {
		return Type::EXTERNAL;
	}
}
