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
#include "process.h"
#include "defer.h"


namespace Command
{
	struct Pipe
	{
		HANDLE toWrite;
		HANDLE toRead;
	};

	struct ProcessIoHandle
	{
		std::shared_ptr<Process> proc;
		std::vector<HANDLE> handles;
	};

	struct StdHandles
	{
		HANDLE in;
		HANDLE out;
		HANDLE err;
	};

	enum class Status
	{
		Nothing,
		Success,
		Failure
	};


	// OpenFile for > or >>
	Result<HANDLE> OpenFileForWrite(const std::string& path, bool append);
	// OpenFile for <
	Result<HANDLE> OpenFileForRead(const std::string& path);
	// OpenPipe for |
	Result<Pipe> OpenPipe();
	// Bulk close pipes
	void ClosePipesRead(const std::vector<Pipe>& pipes);
	void ClosePipesWrite(const std::vector<Pipe>& pipes);
	Result<StdHandles> DuplicateStdHandles();

	// | pipeline | pipeline | ...
	std::vector<std::vector<Command::Node>> SplitToPipeline(const std::vector<Command::Node>& nodes);
	// split by & or && or ||
	std::vector<std::vector<Command::Node>> SplitByExecutionFlowOperator(const std::vector<Command::Node>& nodes);
	// split by |
	std::vector<std::vector<Command::Node>> SplitByPipeOperator(const std::vector<Command::Node>& nodes);

	Result<DWORD> ExecuteCommand(const std::vector<Command::Node>& nodes, HANDLE inHandle, HANDLE outHandle, HANDLE errHandle);

	Result<ProcessIoHandle> AsyncExecuteCommand(const std::vector<Command::Node>& nodes, HANDLE inHandle, HANDLE outHandle, HANDLE errHandle);

	void ShowError(DWORD code, HANDLE handle);

	std::string GetErrorMessage(DWORD code);




	OptionalError Execute(const std::string& input)
	{
		// TODO
		// * More?
		// * Error handling and show error message.
		// * 2>&1
		// * return exit code
		auto [nodes, err] = Parse(input);
		if (err) {
			//fprintf(stderr, "The syntax of the command is incorrect.");
			return Error(err->code, "The syntax of the command is incorrect.");
		}

		// TODO split to block
		// echo foo > file | cat  = echo foo | cat
		// pipeline
		DWORD exitCode = 0;

		for (auto& pipeline : SplitToPipeline(*nodes)) {
			Status status = Status::Nothing;
			// execution-unit ::= command | command pipe execution-unit | command redirection pipe execution-unit
			// redirection ::= redirector file | redirectior file redirection
			// redirector  ::= ">" | ">>" | "<"
			// pipe ::= "|"
			auto combinedCommands = SplitByExecutionFlowOperator(pipeline);
			for (auto combinedCommand = combinedCommands.begin(); combinedCommand != combinedCommands.end(); combinedCommand++) {
				
				auto executionUnits = SplitByPipeOperator(*combinedCommand);
				std::vector<ProcessIoHandle> processes;
				std::optional<Pipe> prevPipe = std::nullopt;
				
				for (auto commands = executionUnits.begin(); commands != executionUnits.end(); commands++) {
					// handles for child process
					auto [handles, err] = DuplicateStdHandles();
					if (err) {
						return err;
					}

					// input
					if (prevPipe) {
						::CloseHandle(handles->in);
						handles->in = prevPipe->toRead;
					}

					// output and next input
					if (commands->back().type == NodeType::Pipe) {
						auto [pipe, pipeErr] = OpenPipe();
						if (pipeErr) return pipeErr;
						::CloseHandle(handles->out);
						handles->out = pipe->toWrite;
						prevPipe = pipe;
					}
	

					if (commands + 1 != executionUnits.end()) {
						// mid
						auto [process, err] = AsyncExecuteCommand(*commands, handles->in, handles->out, handles->err);
						if (err) {
							return err;
						}
						processes.push_back(*process);
					}
					else {
						// at last
						auto [process, err] = AsyncExecuteCommand(*commands, handles->in, handles->out, handles->err);
						if (err) {
							return err;
						}

						// wait for exit processes
						for (auto& each : processes) {
							auto [code, err] = each.proc->WaitForExit();
							if (err) {
								return err;
							}
							for(auto& h : each.handles) {
								::CloseHandle(h);
							}
						}
						
						// wait for exit current process
						auto [code, waitErr] = process->proc->WaitForExit();
						if (waitErr) {
							return err;
						}
						for (auto& h : process->handles) {
							::CloseHandle(h);
						}

	
						exitCode = *code;
						status = exitCode == ERROR_SUCCESS ? Status::Success : Status::Failure;
						if (exitCode != ERROR_SUCCESS) {
							ShowError(exitCode, handles->err);
						}
					}
				}
			}
			
		}

		return std::nullopt;
	}

	std::vector<std::vector<Command::Node>> SplitToPipeline(const std::vector<Command::Node>& nodes)
	{
		std::vector<std::vector<Command::Node>> result{};
		std::vector<Command::Node> pipeline{};
		for (auto& node : nodes) {
			/*if (node.type == NodeType::Pipe) {
				result.push_back(pipeline);
				pipeline = {};
			}
			else {*/
				pipeline.push_back(node);
			//}
		}
		if (0 < pipeline.size()) {
			result.push_back(pipeline);
		}
		return result;
	}


	std::vector<std::vector<Command::Node>> SplitByExecutionFlowOperator(const std::vector<Command::Node>& nodes)
	{
		// -- result --
		// {
		//   { "commandA",  "|",  "commandB",  "&&" },
		//   { "commandC",  ">",  "fileD",     "&"  },
		//   { "commandE",  "<",  "fileF",     "|"  },
		//   { "commandG"                           }
		// }
		std::vector<std::vector<Command::Node>> result{};
		std::vector<Command::Node> chunk{};
		for (auto& node : nodes) {
			if (node.type == NodeType::And || node.type == NodeType::Separator || node.type == NodeType::Or) {
				result.push_back(chunk);
				result.push_back({ node });
				chunk = {}; // renew
			}
			else {
				chunk.push_back(node);
			}
		}
		if (0 < chunk.size()) {
			result.push_back(chunk);
		}
		return result;
	}


	std::vector<std::vector<Command::Node>> SplitByPipeOperator(const std::vector<Command::Node>& nodes)
	{
		// -- result --
		// {
		//   { "commandA",  ">",  "fileB",  "|" },
		//   { "commandC",  "<",  "fileD",  "|" },
		//   { "commandE",  ">",  "fileF",  "<",  "FileG",  "|"  },
		//   { "commandG" }
		// }
		std::vector<std::vector<Command::Node>> result{};
		std::vector<Command::Node> chunk{};
		for (auto& node : nodes) {
			if (node.type == NodeType::Pipe) {
				chunk.push_back(node);
				result.push_back(chunk);
				chunk = {}; // renew
			}
			else {
				chunk.push_back(node);
			}
		}
		if (0 < chunk.size()) {
			result.push_back(chunk);
		}
		return result;
	}

	
	Result<DWORD> ExecuteCommand(const std::vector<Command::Node>& nodes, HANDLE inHandle, HANDLE outHandle, HANDLE errHandle)
	{
		std::vector<HANDLE> handles{};
		DEFER([&handles]() 
			{
				for (HANDLE& handle : handles) {
					::OutputDebugStringA("CloseHandles\n");
					::CloseHandle(handle);
				}
			}
		);
		Node command = nodes.at(0);
		for (size_t i = 1; i < nodes.size(); i++) {
			if (nodes.at(i).type == NodeType::Redirect) {
				auto [handle, err] = OpenFileForWrite(nodes.at(i + 1).file, false);
				if (err) return { std::nullopt, err };
				handles.push_back(*handle);
				outHandle = *handle;
			}
			else if (nodes.at(i).type == NodeType::Append) {
				auto [handle, err] = OpenFileForWrite(nodes.at(i + 1).file, true);
				if (err) return { std::nullopt, err };
				handles.push_back(*handle);
				outHandle = *handle;
			}
			else if (nodes.at(i).type == NodeType::Input) {
				auto [handle, err] = OpenFileForRead(nodes.at(i + 1).file);
				if (err) return { std::nullopt, err };
				handles.push_back(*handle);
				inHandle = *handle;
			}
		}
		Process p(command.command, command.arguments, inHandle, outHandle, errHandle);
		auto startErr = p.Start();
		if (startErr) return { std::nullopt, startErr };
		auto [code, waitErr] = p.WaitForExit();
		if (waitErr) return { std::nullopt, waitErr };
		return { *code, std::nullopt };
	}


	Result<ProcessIoHandle> AsyncExecuteCommand(const std::vector<Command::Node>& nodes, HANDLE inHandle, HANDLE outHandle, HANDLE errHandle)
	{
		std::vector<HANDLE> handles{};
		Node command = nodes.at(0);
		for (size_t i = 1; i < nodes.size(); i++) {
			if (nodes.at(i).type == NodeType::Redirect) {
				auto [handle, err] = OpenFileForWrite(nodes.at(i + 1).file, false);
				if (err) return { std::nullopt, err };
				handles.push_back(*handle);
				outHandle = *handle;
			}
			else if (nodes.at(i).type == NodeType::Append) {
				auto [handle, err] = OpenFileForWrite(nodes.at(i + 1).file, true);
				if (err) return { std::nullopt, err };
				handles.push_back(*handle);
				outHandle = *handle;
			}
			else if (nodes.at(i).type == NodeType::Input) {
				auto [handle, err] = OpenFileForRead(nodes.at(i + 1).file);
				if (err) return { std::nullopt, err };
				handles.push_back(*handle);
				inHandle = *handle;
			}
		}
		auto proc = std::make_shared<Process>(command.command, command.arguments, inHandle, outHandle, errHandle);
		// important
		::CloseHandle(inHandle);
		::CloseHandle(outHandle);
		::CloseHandle(errHandle);

		auto startErr = proc->Start();
		if (startErr) return { std::nullopt, startErr };
		return { ProcessIoHandle{ proc, handles }, std::nullopt };
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
			return { std::nullopt, Error(err, "Failed to CreateFile OpenFileForWrite@command.cpp") };
		}
		if (append) {
			if (::SetFilePointer(h, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER){
				auto err = ::GetLastError();
				return { std::nullopt, Error(err, "Failed to ::SetFilePointer OpenFileForWrite@command.cpp") };
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
			return { std::nullopt, Error(err, "Failed to CreateFile OpenFileForRead@command.cpp") };
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
		if (!::CreatePipe(&read, &write, &security, 0)) {
			return { std::nullopt, Error(::GetLastError(),  "Failed to ::CreatePipe OpenPipe@command.cpp") };
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

	void ClosePipesRead(const std::vector<Pipe>& pipes)
	{
		for (auto& pipe : pipes) {
			::CloseHandle(pipe.toRead);
		}
	}
	void ClosePipesWrite(const std::vector<Pipe>& pipes)
	{
		for (auto& pipe : pipes) {
			::CloseHandle(pipe.toWrite);
		}
	}

	Result<StdHandles> DuplicateStdHandles()
	{
		HANDLE originalInput = ::GetStdHandle(STD_INPUT_HANDLE);
		HANDLE originalOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE originalError = ::GetStdHandle(STD_ERROR_HANDLE);
		HANDLE in, out, err;
		BOOL res = ::DuplicateHandle(
			::GetCurrentProcess(),
			originalInput,
			::GetCurrentProcess(),
			&in,
			0,
			TRUE,
			DUPLICATE_SAME_ACCESS
		);
		if (!res) {
			return { std::nullopt, Error(::GetLastError(), "Failed to DuplicateHandle DuplicateStdHandles@command.cpp") };
		}
		res = ::DuplicateHandle(
			::GetCurrentProcess(),
			originalOutput,
			::GetCurrentProcess(),
			&out,
			0,
			TRUE,
			DUPLICATE_SAME_ACCESS
		);
		if (!res) {
			return { std::nullopt, Error(::GetLastError(), "Failed to DuplicateHandle DuplicateStdHandles@command.cpp") };
		}
		res = ::DuplicateHandle(
			::GetCurrentProcess(),
			originalError,
			::GetCurrentProcess(),
			&err,
			0,
			TRUE,
			DUPLICATE_SAME_ACCESS
		);
		if (!res) {
			return { std::nullopt, Error(::GetLastError(), "Failed to DuplicateHandle DuplicateStdHandles@command.cpp") };
		}
		return { StdHandles{ in, out, err}, std::nullopt };

	}
}
