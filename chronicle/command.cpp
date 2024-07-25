#include "command.h"
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <cctype>

#include "tokenize.h"
#include "parser.h"
#include "node.h"
#include "error.h"
#include "result.h"
#include "process.h"
#include "defer.h"


namespace Command
{
	// PipeHandle read, write
	struct Pipe
	{
		HANDLE toWrite;
		HANDLE toRead;
	};

	// OpenFile for > or >>
	Result<HANDLE> OpenFileForWrite(const std::wstring& path, bool append);
	// OpenFile for <
	Result<HANDLE> OpenFileForRead(const std::wstring& path);
	// OpenPipe for |
	Result<Pipe> OpenPipe();

	// command && command
	Result<DWORD> ExecuteCommandSequence(std::shared_ptr<Node> node, bool doNextCommand, DWORD exitCode);

	// command | command
	Result<DWORD> ExecuteCombinedCommand(std::shared_ptr<Node> node);

	// command > file
	std::tuple<std::shared_ptr<Process>, OptionalError> AsyncExecuteCommand(std::shared_ptr<Node> node, HANDLE inHandle, HANDLE outHandle, HANDLE errHandle);

	void ShowError(DWORD code, HANDLE handle);

	std::string GetErrorMessage(DWORD code);

	


	Result<DWORD> Execute(const std::wstring& input)
	{
		//// TODO
		//// * More?
		//// * Error handling and show error message.
		//// * 2>&1
		//// * return exit code
		//// * expand environment variables
		auto [tokens, tokenErr] = Tokenize(input);
		if (tokenErr) {
			return { std::nullopt, tokenErr };
		}

		Parser parser(*tokens);
		auto [node, parseErr] = parser.Parse();
		if (parseErr) {
			return { std::nullopt, parseErr };
		}
		

		auto [code, execErr] = ExecuteCommandSequence(node, true, 0);

		return { code, std::nullopt };
	}


	
	Result<DWORD> ExecuteCommandSequence(std::shared_ptr<Node> node, bool doNextCommand, DWORD exitCode)
	{
		switch (node->type)
		{
		case NodeType::And:
		{
			// succes && fail && ???
			if (doNextCommand) {
				auto [code, err] = ExecuteCombinedCommand(node->left);
				if (err) {
					return { std::nullopt, err };
				}
				auto result = *code == ERROR_SUCCESS;
				return ExecuteCommandSequence(node->right, result, *code);
			}
			else {
				return ExecuteCommandSequence(node->right, false, exitCode);
			}
			break;
		}
		case NodeType::Or:
		{
			// cmdA || cmdB || cmdC
			if (doNextCommand) {
				auto [code, err] = ExecuteCombinedCommand(node->left);
				if (err) {
					return { std::nullopt, err };
				}
				auto result = *code != ERROR_SUCCESS;
				return ExecuteCommandSequence(node->right, result, *code);
			}
			else {
				return ExecuteCommandSequence(node->right, false, exitCode);
			}
			break;
		}
		case NodeType::Separator:
		{
			// cmdA & cmd B
			if (doNextCommand) {
				auto [code, err] = ExecuteCombinedCommand(node->left);
				if (err) {
					return { std::nullopt, err };
				}
				return ExecuteCommandSequence(node->right, true, *code);
			}
			else {
				return ExecuteCommandSequence(node->right, true, exitCode);
			}
			break;
		}
		default:
			if (doNextCommand) {
				return ExecuteCombinedCommand(node);
			}
			else {
				return { exitCode, std::nullopt };
			}
		}
	}


	Result<DWORD> ExecuteCombinedCommand(std::shared_ptr<Node> node)
	{
		// collect command
		std::vector<std::shared_ptr<Node>> commands{};
		auto current = node;
		while (true) {
			if (current->type != NodeType::Pipe && current->type != NodeType::Command) {
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, L"Unexpected NodeType ExecuteBombinedCommand@command") };
			}

			// NodeType::Command
			if (current->type == NodeType::Command) {
				commands.push_back(current);
				break;
			}
			// NodeType::Pipe
			if (!current->left || current->left->type != NodeType::Command) {
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, L"Unexpected NodeType ExecuteBombinedCommand@command") };
			}
			commands.push_back(current->left);
			current = current->right;
		}

		HANDLE inHandle = ::GetStdHandle(STD_INPUT_HANDLE);
		HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE errHandle = ::GetStdHandle(STD_ERROR_HANDLE);

		// Specialization of Single Command
		if (commands.size() == 1) {
			auto [process, executeErr] = AsyncExecuteCommand(commands.at(0), inHandle, outHandle, errHandle);
			if (executeErr) {
				return { std::nullopt, executeErr };
			}
			auto [code, err] = process->WaitForExit();
			if (err) {
				return { std::nullopt, err };
			}
			return { *code, std::nullopt };
		}


		// Execute Commands
		std::vector<std::shared_ptr<Process>> processes{};
		std::optional<Pipe> prevPipe = std::nullopt;

		// First Command
		{
			auto [pipe, pipeErr] = OpenPipe();
			if (pipeErr) {
				return { std::nullopt, pipeErr };
			}
			auto [proc, executeErr] = AsyncExecuteCommand(commands.front(), inHandle, pipe->toWrite, errHandle);
			if (executeErr) {
				return { std::nullopt, executeErr };
			}
			::CloseHandle(pipe->toWrite);
			processes.push_back(proc);
			prevPipe = pipe;
		}

		// Middle
		for (size_t i = 1; i < commands.size() - 1; i++) {
			// pipe
			auto [pipe, pipeErr] = OpenPipe();
			if (pipeErr) {
				return { std::nullopt, pipeErr };
			}
			
			// check
			if (!prevPipe) {
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, L"LogicalError ExecuteBombinedCommand@command")};
			}

			// Execute
			auto [proc, executeErr] = AsyncExecuteCommand(commands.at(i), prevPipe->toRead, pipe->toWrite, errHandle);
			if (executeErr) {
				return { std::nullopt, executeErr };
			}
			::CloseHandle(prevPipe->toRead);
			::CloseHandle(pipe->toWrite);
			processes.push_back(proc);
			prevPipe = pipe;
		}

		// Last
		{
			auto [proc, executeErr] = AsyncExecuteCommand(commands.back(), prevPipe->toRead, outHandle, errHandle);
			if (executeErr) {
				return { std::nullopt, executeErr };
			}
			::CloseHandle(prevPipe->toRead);
			processes.push_back(proc);
		}

		// wait for exit processes
		DWORD exitCode = 0;
		for (auto& proc : processes) {
			auto [code, err] = proc->WaitForExit();
			if (err) {
				return { std::nullopt, err };
			}
			exitCode = *code;
		}
		return { exitCode,  std::nullopt };		
	}


	std::tuple<std::shared_ptr<Process>, OptionalError> AsyncExecuteCommand(std::shared_ptr<Node> node, HANDLE inHandle, HANDLE outHandle, HANDLE errHandle)
	{
		std::vector<HANDLE> handles{};
		std::wstring inFile = L"";
		std::wstring outFile = L"";
		std::wstring errFile = L"";
		bool outAppend = false;
		bool errAppend = false;
		for (auto& redirect : node->redirections) {
			if (redirect.op == L"<") {
				inFile = redirect.file;
			}
			else if (redirect.op == L">") {
				outFile = redirect.file;
				outAppend = false;
			}
			else if (redirect.op == L">>") {
				outFile = redirect.file;
				outAppend = true;
			}
			else if (redirect.op == L"1>") {
				outFile = redirect.file;
				outAppend = false;
			}
			else if (redirect.op == L"1>>") {
				outFile = redirect.file;
				outAppend = true;
			}
			else if (redirect.op == L"2>") {
				errFile = redirect.file;
				errAppend = false;

			}
			else if (redirect.op == L"2>>") {
				errFile = redirect.file;
				errAppend = true;
			}
		}
		if (!inFile.empty()) {
			auto [handle, err] = OpenFileForRead(inFile);
			if (err) return { nullptr, err };
			handles.push_back(*handle);
			inHandle = *handle;
		}
		if (!outFile.empty()) {
			auto [handle, err] = OpenFileForWrite(outFile, outAppend);
			if (err) return { nullptr, err };
			handles.push_back(*handle);
			outHandle = *handle;
		}
		if (!errFile.empty()) {
			auto [handle, err] = OpenFileForWrite(errFile, errAppend);
			if (err) return { nullptr, err };
			handles.push_back(*handle);
			outHandle = *handle;
		}

		auto proc = std::make_shared<Process>(node->command, node->arguments, inHandle, outHandle, errHandle);
		auto startErr = proc->Start();
		
		// After passing the handle to the process, the handle is no longer needed, so close it.
		// Need to close before start next process. It makes input handle will close correctly.
		for (auto& each : handles) {
			::CloseHandle(each);
		}

		if (startErr) return { nullptr, startErr };
		return { proc, std::nullopt };
	}


	Result<HANDLE> OpenFileForWrite(const std::wstring& path, bool append) {
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = nullptr;
		sa.bInheritHandle = TRUE;

		DWORD creationDisposition = 0;
		std::wstring actualPath;
		if (_wcsicmp(path.c_str(), L"NUL") == 0) {
			creationDisposition = OPEN_EXISTING;
			actualPath = L"NUL";
		}
		else {
			creationDisposition = append ? OPEN_ALWAYS : CREATE_ALWAYS;
			DWORD fullPathLength = ::GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
			std::wstring buf(fullPathLength + 1, '/0');
			::GetFullPathNameW(path.c_str(), buf.size(), buf.data(), nullptr);
			actualPath = L"\\\\?\\" + buf;
		}
		

		HANDLE h = ::CreateFileW(
			actualPath.c_str(), 
			GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa,
			creationDisposition,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
		if (h == INVALID_HANDLE_VALUE) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, L"Failed to CreateFile OpenFileForWrite@command") };
		}
		if (append) {
			if (::SetFilePointer(h, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER){
				auto err = ::GetLastError();
				return { std::nullopt, Error(err, L"Failed to ::SetFilePointer OpenFileForWrite@command") };
			}
		}
		return { h, std::nullopt };
	}


	Result<HANDLE> OpenFileForRead(const std::wstring& path) {
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = nullptr;
		sa.bInheritHandle = TRUE;

		DWORD fullPathLength = ::GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
		std::wstring buf(fullPathLength + 1, '/0');
		::GetFullPathNameW(path.c_str(), buf.size(), buf.data(), nullptr);
		std::wstring actualPath = L"\\\\?\\" + buf;
		
		HANDLE h = ::CreateFileW(
			actualPath.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
		if (h == INVALID_HANDLE_VALUE) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, L"Failed to CreateFile OpenFileForRead@command") };
		}
		return { h, std::nullopt };
	}


	Result<Pipe> OpenPipe()
	{
		// Security Attributes to use handle by child process
		SECURITY_ATTRIBUTES security{};
		security.nLength = sizeof(SECURITY_ATTRIBUTES);
		security.bInheritHandle = TRUE;

		HANDLE read{};
		HANDLE write{};
		if (!::CreatePipe(&read, &write, & security, 0)) {
			return { std::nullopt, Error(::GetLastError(),  L"Failed to ::CreatePipe OpenPipe@command") };
		}

		// To use handle in child proces
		//::SetHandleInformation(read, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		//::SetHandleInformation(write, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

		return { Pipe{ write, read }, std::nullopt };
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
			reinterpret_cast<char*>(& buf),
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


}
