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
	Result<HANDLE> OpenFileForWrite(const std::string& path, bool append);
	// OpenFile for <
	Result<HANDLE> OpenFileForRead(const std::string& path);
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

	


	Result<DWORD> Execute(const std::string& input)
	{
		//// TODO
		//// * More?
		//// * Error handling and show error message.
		//// * 2>&1
		//// * return exit code
		//// * expand 
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



		//// TODO split to block
		//// echo foo > file | cat  = echo foo | cat
		//// pipeline
		//DWORD exitCode = 0;

		//for (auto& pipeline : SplitToPipeline(*nodes)) {
		//	// execution-unit ::= command | command pipe execution-unit | command redirection pipe execution-unit
		//	// redirection ::= redirector file | redirectior file redirection
		//	// redirector  ::= ">" | ">>" | "<"
		//	// pipe ::= "|"
		//
		//	auto combinedCommands = SplitByFlowOperator(pipeline);
		//	for (auto combinedCommand = combinedCommands.begin(); combinedCommand != combinedCommands.end(); combinedCommand++) {
		//		
		//		auto executionUnits = SplitByPipeOperator(*combinedCommand);
		//		
		//		Status status = Status::Nothing;
		//		std::vector<ProcessIoHandle> processes;
		//		std::optional<Pipe> prevPipe = std::nullopt;
		//		
		//		// commands e.g.
		//		//   {
		//		//      { cd },
		//		//      { echo hello world },
		//		//      { dir | },
		//		//      { findstr b | }
		//		//      { echo hello file > filename.txt && }
		//		//   }
		//		//  
		//		for (auto commands = executionUnits.begin(); commands != executionUnits.end(); commands++) {
		//			// handles for child process
		//			auto [handles, err] = DuplicateStdHandles();
		//			if (err) {
		//				return err;
		//			}
		//
		//			// input
		//			if (prevPipe) {
		//				::CloseHandle(handles->in);
		//				handles->in = prevPipe->toRead;
		//			}
		//
		//			// output and next input
		//			if (commands->back().type == NodeType::Pipe) {
		//				auto [pipe, pipeErr] = OpenPipe();
		//				if (pipeErr) return pipeErr;
		//				::CloseHandle(handles->out);
		//				handles->out = pipe->toWrite;
		//				prevPipe = pipe;
		//			}
		//
		//			
		//			if (commands + 1 != executionUnits.end()) {
		//				// left
		//				auto [process, err] = AsyncExecuteCommand(*commands, handles->in, handles->out, handles->err);
		//				if (err) {
		//					return err;
		//				}
		//				processes.push_back(*process);
		//			}
		//			else {
		//				// at last ----
		//				auto [process, err] = AsyncExecuteCommand(*commands, handles->in, handles->out, handles->err);
		//				if (err) {
		//					return err;
		//				}
		//
		//				// wait for exit processes
		//				for (auto& each : processes) {
		//					auto [code, err] = each.proc->WaitForExit();
		//					if (err) {
		//						return err;
		//					}
		//					for(auto& h : each.handles) {
		//						::CloseHandle(h);
		//					}
		//				}
		//				
		//				// wait for exit current process
		//				auto [code, waitErr] = process->proc->WaitForExit();
		//				if (waitErr) {
		//					return err;
		//				}
		//				for (auto& h : process->handles) {
		//					::CloseHandle(h);
		//				}
		//
		//
		//				exitCode = *code;
		//				status = exitCode == ERROR_SUCCESS ? Status::Success : Status::Failure;
		//				if (exitCode != ERROR_SUCCESS) {
		//					ShowError(exitCode, handles->err);
		//				}
		//
		//			}
		//
		//		}
		//	
		//
		//		// check operator
		//		switch (combinedCommand->back().type)
		//		{
		//		case NodeType::And:
		//			// "&&" operator
		//			if (status == Status::Failure) {
		//				continue;
		//			}
		//			break;
		//		case NodeType::Or:
		//			// "||" operator
		//			if (status == Status::Success) {
		//				continue;
		//			}
		//			break;
		//		case NodeType::Separator:
		//			// "&" operator
		//			break;
		//		default:
		//			break;
		//			//return Error(ERROR_INVALID_FUNCTION, "LogicalError Execute@command.cpp");
		//		}
		//	}
		//	
		//}

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
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "Unexpected NodeType ExecuteBombinedCommand@command") };
			}

			// NodeType::Command
			if (current->type == NodeType::Command) {
				commands.push_back(current);
				break;
			}
			// NodeType::Pipe
			if (!current->left || current->left->type != NodeType::Command) {
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "Unexpected NodeType ExecuteBombinedCommand@command") };
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
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "LogicalError ExecuteBombinedCommand@command")};
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
		std::string inFile = "";
		std::string outFile = "";
		std::string errFile = "";
		bool outAppend = false;
		bool errAppend = false;
		for (auto& redirect : node->redirections) {
			if (redirect.op == "<") {
				inFile = redirect.file;
			}
			else if (redirect.op == ">") {
				outFile = redirect.file;
				outAppend = false;
			}
			else if (redirect.op == ">>") {
				outFile = redirect.file;
				outAppend = true;
			}
			else if (redirect.op == "1>") {
				outFile = redirect.file;
				outAppend = false;
			}
			else if (redirect.op == "1>>") {
				outFile = redirect.file;
				outAppend = true;
			}
			else if (redirect.op == "2>") {
				errFile = redirect.file;
				errAppend = false;

			}
			else if (redirect.op == "2>>") {
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


	Result<HANDLE> OpenFileForWrite(const std::string& path, bool append) {
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = nullptr;
		sa.bInheritHandle = TRUE;

		DWORD creationDisposition = 0;
		std::string actualPath;
		if (_stricmp(path.c_str(), "NUL") == 0) {
			creationDisposition = OPEN_EXISTING;
			actualPath = "NUL";
		}
		else {
			creationDisposition = append ? OPEN_ALWAYS : CREATE_ALWAYS;
			DWORD fullPathLength = ::GetFullPathNameA(path.c_str(), 0, nullptr, nullptr);
			std::string buf(fullPathLength + 1, '/0');
			::GetFullPathNameA(path.c_str(), buf.size(), buf.data(), nullptr);
			actualPath = "\\\\?\\" + buf;
		}
		

		HANDLE h = ::CreateFileA(
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
			return { std::nullopt, Error(err, "Failed to CreateFile OpenFileForWrite@command") };
		}
		if (append) {
			if (::SetFilePointer(h, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER){
				auto err = ::GetLastError();
				return { std::nullopt, Error(err, "Failed to ::SetFilePointer OpenFileForWrite@command") };
			}
		}
		return { h, std::nullopt };
	}


	Result<HANDLE> OpenFileForRead(const std::string& path) {
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = nullptr;
		sa.bInheritHandle = TRUE;

		DWORD fullPathLength = ::GetFullPathNameA(path.c_str(), 0, nullptr, nullptr);
		std::string buf(fullPathLength + 1, '/0');
		::GetFullPathNameA(path.c_str(), buf.size(), buf.data(), nullptr);
		std::string actualPath = "\\\\?\\" + buf;
		
		HANDLE h = ::CreateFileA(
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
			return { std::nullopt, Error(err, "Failed to CreateFile OpenFileForRead@command") };
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
			return { std::nullopt, Error(::GetLastError(),  "Failed to ::CreatePipe OpenPipe@command") };
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
