#include "process.h"
#include <string>
#include <vector>
#include "internalcommand.h"
#include "result.h"
#include "error.h"
#include "stringutil.h"


// functions
Type ClassifyCommand(const std::wstring& command);
void ShowError(DWORD code, HANDLE handle);
std::string GetErrorMessage(DWORD code);



Process::Process(const std::wstring& cmd, const std::wstring& arg, HANDLE in, HANDLE out, HANDLE err)
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
	if (this->type == Type::EXTERNAL) {
		::CloseHandle(this->processInfo.hProcess);
		::CloseHandle(this->processInfo.hThread);
	}
}


OptionalError Process::Start()
{
	
	if (type == Type::EXTERNAL) {
		DWORD result = this->Create();
		if (result != ERROR_SUCCESS) {
			// TODO not the same as cmd.exe
			ShowError(result, this->error);
		}
		return std::nullopt;
	}
	else {
		// 
		//BOOL res = ::DuplicateHandle(
		//	::GetCurrentProcess(),
		//	this->input,
		//	::GetCurrentProcess(),
		//	&this->input,
		//	0,
		//	TRUE,
		//	DUPLICATE_SAME_ACCESS /*| DUPLICATE_CLOSE_SOURCE*/
		//);
		//if (!res) {
		//	return Error(::GetLastError(), "Failed to DuplicateHandle Start@process.cpp");
		//}
		//res = ::DuplicateHandle(
		//	::GetCurrentProcess(),
		//	this->output,
		//	::GetCurrentProcess(),
		//	&this->output,
		//	0,
		//	TRUE,
		//	DUPLICATE_SAME_ACCESS
		//);
		//if (!res) {
		//	return Error(::GetLastError(), "Failed to DuplicateHandle Start@process.cpp");
		//}
		//res = ::DuplicateHandle(
		//	::GetCurrentProcess(),
		//	this->error,
		//	::GetCurrentProcess(),
		//	&this->error,
		//	0,
		//	TRUE,
		//	DUPLICATE_SAME_ACCESS
		//);
		//if (!res) {
		//	return Error(::GetLastError(), "Failed to DuplicateHandle Start@process.cpp");
		//}
		
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
		auto [code, err] = InternalCommand::Cd(L"/D " + command, this->output);
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
			return { std::nullopt, Error(err, L"Failed to ::GetExitCodeProcess WaitForExit@process.cpp") };
		}
	}
	else {
		//::CloseHandle(this->input);
		//::CloseHandle(this->output);
		//::CloseHandle(this->error);
	}
	return { exitCode, std::nullopt };
}


DWORD Process::Create()
{
	// launch cmd.exe
	STARTUPINFOA startupInfo{};
	startupInfo.cb = sizeof(startupInfo);

	startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	startupInfo.hStdInput = this->input;
	startupInfo.hStdOutput = this->output;
	startupInfo.hStdError = this->error;

	std::wstring commandAndArguments = this->command + L" " + this->arguments;
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
		return this->exitCode;
	}
	else {
		return ERROR_SUCCESS;
	}
}


void ShowError(DWORD code, HANDLE handle)
{
	std::string message = GetErrorMessage(code);
	DWORD written = 0;
	::WriteFile(handle, message.data(), message.size(), &written, nullptr);
}


std::string GetErrorMessage(DWORD code)
{
	char* buf = 0;
	auto len = ::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<char*>(&buf),
		0,
		nullptr
	);
	// FIXME
	if (len == 0) {
		auto err = ::GetLastError();
		return "";
	}
	std::string result(buf);
	::LocalFree(buf);
	return result;
}




Type ClassifyCommand(const std::wstring& command)
{
	if (command.empty()) {
		return Type::EMPTY;
	}
	else if (_wcsicmp(command.c_str(), L"dir") == 0) {
		return Type::DIR;
	}
	else if (_wcsicmp(command.c_str(), L"cd") == 0) {
		return Type::CD;
	}
	else if (_wcsicmp(command.c_str(), L"pushd") == 0) {
		return Type::PUSHD;
	}
	else if (_wcsicmp(command.c_str(), L"popd") == 0) {
		return Type::POPD;
	}
	else if (_wcsicmp(command.c_str(), L"set") == 0) {
		return Type::SET;
	}
	else if (_wcsicmp(command.c_str(), L"echo") == 0) {
		return Type::ECHO;
	}
	else if (InternalCommand::IsDriveLetter(command)) {
		return Type::CHANGE_DRIVE;
	}
	else {
		return Type::EXTERNAL;
	}
}
