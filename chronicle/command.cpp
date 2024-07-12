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
	// | pipeline | pipeline | ...
	std::vector<std::vector<Command::Node>> SplitToPipeline(const std::vector<Command::Node>& nodes);
	// command & command && command || command & ...
	std::vector<std::vector<Command::Node>> SplitToCommandAndControlOp(const std::vector<Command::Node>& nodes);
	
	Result<DWORD> ExecuteCommand(const std::vector<Command::Node>& nodes, HANDLE inHandle, HANDLE outHandle, HANDLE errHandle);

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
		HANDLE inHandle = ::GetStdHandle(STD_INPUT_HANDLE);
		HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE errHandle = ::GetStdHandle(STD_ERROR_HANDLE);

		// echo Hello && dir | find "file" | sort > sorted_files.txt
		// TODO split to block
		// pipeline
		DWORD exitCode = 0;
		for (auto& pipeline : SplitToPipeline(*nodes)) {
			// 
			Status status = Status::Nothing;
			auto commands = SplitToCommandAndControlOp(pipeline);
			// command > file && command < file & command
			bool skip = false;
			for (size_t i = 0; i < commands.size(); i++) {
				if (skip) {
					skip = false;
				}
				else {
					auto [code, err] = ExecuteCommand(commands.at(i), inHandle, outHandle, errHandle);
					if (err) {
						return err;
					}
					exitCode = *code;
					status = exitCode == ERROR_SUCCESS ? Status::Success : Status::Failure;
					if (exitCode != ERROR_SUCCESS) {
						ShowError(exitCode, errHandle);
					}
				}
				
				// look at operator
				if (commands.size() <= i + 1) {
					continue;
				}
				if (commands.at(i + 1).at(0).type == NodeType::And) {
					if (status == Status::Failure) {
						skip = true;
					}
					i++;
				}
				else if (commands.at(i + 1).at(0).type == NodeType::Or) {
					if (status == Status::Success) {
						skip = true;
					}
					i++;
				}
				else if (commands.at(i + 1).at(0).type == NodeType::Separator) {
					i++;
				}
				else {
					return Error(ERROR_INVALID_FUNCTION, "LogicalError Execute@command.cpp");
				}
			}
		}
		return std::nullopt;
		// redirection
		// command

		// current process
		// process list?
		// current 1 (stdout)
		// current 2 (stderr)
		// >, >>, &, &&, ||, | 
		// 2>&1
		std::vector<std::unique_ptr<Process>> processes;
		std::vector<HANDLE> files;
		std::vector<Pipe> pipes;

		// prepare processes
		/*
		for (size_t i = 0; i < nodes->size(); i++)
		{
			switch (nodes->at(i).type)
			{
			case NodeType::End:
				break;
				//return std::nullopt;
			case NodeType::Command:
			{
				// Redirect  ">" or ">>"
				if (nodes->at(i + 1).type == NodeType::Redirect || nodes->at(i + 1).type == NodeType::Append) {
					if (nodes->at(i + 2).type != NodeType::File) {
						return Error(ERROR_INVALID_FUNCTION, "LogicalError Execute@command.cpp");
					}
					bool append = nodes->at(i + 1).type == NodeType::Append;
					auto [handle, err] = OpenFile(nodes->at(i + 2).file, append);
					if (err) {
						return err;
					}
					else {
						files.push_back(*handle);
					}
					processes.push_back(std::make_unique<Process>(nodes->at(i).command, nodes->at(i).arguments, inHandle, *handle, errHandle));
					i += 2;
				}
				// Pipe "|"
				else if (nodes->at(i + 1).type == NodeType::Pipe) {
					if (nodes->at(i + 2).type != NodeType::Command) {
						return Error(ERROR_INVALID_FUNCTION, "LogicalError Execute@command.cpp");
					}
					auto [pipe, err] = OpenPipe();
					if (err) {
						return err;
					}
					pipes.push_back(*pipe);
					processes.push_back(std::make_unique<Process>(nodes->at(i).command, nodes->at(i).arguments, inHandle, pipe->toWrite, errHandle));
					processes.push_back(std::make_unique<Process>(nodes->at(i+2).command, nodes->at(i+2).arguments, pipe->toRead, outHandle, errHandle));
					i += 2;
				}
				else {
					processes.push_back(std::make_unique<Process>(nodes->at(i).command, nodes->at(i).arguments, inHandle, outHandle, errHandle));
					//ExecuteCommand(nodes->at(i).command, nodes->at(i).arguments);
				}
			}
			}
		}

		// start process
		for (auto& process : processes) {
			// TODO
			// file locked or etc...
			auto startErr = process->Start();
			if (startErr) {
				if (startErr->code == ERROR_FILE_NOT_FOUND) {
					fprintf(stderr, "'%s' is not recognized as an internal or external command,\n", process->command.c_str());
				}
				auto msg = GetErrorMessage(startErr->code);
				fprintf(stderr, "%s\n", msg.c_str());
				puts("");
				continue;
			}
			auto [exitCode, err] = process->WaitForExit();
			if (err) {
				auto msg = GetErrorMessage(err->code);
				fprintf(stderr, "%s", msg.c_str()); // ErrorMassage coutains \r\n
			}
			else {
				puts("");
			}
		}
		//processes[1]->Start();
		//processes[0]->Start();
		//processes[0]->WaitForExit();
		//processes[1]->WaitForExit();
		for (auto file : files) {
			::CloseHandle(file);
		}
		for (auto& pipe : pipes) {
			::CloseHandle(pipe.toWrite);
			::CloseHandle(pipe.toRead);
		}

		return std::nullopt;
		*/

		//puts("");
		//return result;
	}

	std::vector<std::vector<Command::Node>> SplitToPipeline(const std::vector<Command::Node>& nodes)
	{
		std::vector<std::vector<Command::Node>> result{};
		std::vector<Command::Node> pipeline{};
		for (auto& node : nodes) {
			if (node.type == NodeType::Pipe) {
				result.push_back(pipeline);
				pipeline = {};
			}
			else {
				pipeline.push_back(node);
			}
		}
		if (0 < pipeline.size()) {
			result.push_back(pipeline);
		}
		return result;
	}


	std::vector<std::vector<Command::Node>> SplitToCommandAndControlOp(const std::vector<Command::Node>& nodes)
	{
		std::vector<std::vector<Command::Node>> result{};
		std::vector<Command::Node> commandAndRedirect{};
		for (auto& node : nodes) {
			if (node.type == NodeType::And || node.type == NodeType::Separator || node.type == NodeType::Or) {
				result.push_back(commandAndRedirect);
				result.push_back({ node });
				commandAndRedirect = {}; // renew
			}
			else {
				commandAndRedirect.push_back(node);
			}
		}
		if (0 < commandAndRedirect.size()) {
			result.push_back(commandAndRedirect);
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
		// ? 
		::SetHandleInformation(read, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		::SetHandleInformation(write, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

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


