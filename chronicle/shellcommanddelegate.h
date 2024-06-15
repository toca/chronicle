#pragma once
#include <functional>
#include <string>
#include <thread>
#include "error.h"
#include "result.h"
class ShellCommandDelegate
{
public:
	static Result<ShellCommandDelegate*> Create();
	~ShellCommandDelegate();
	std::optional<Error> Input(const std::string& line);
	void Exit();
	void OnExit(std::function<void()> callback);
	void Break();
	void Wait();
private:
	ShellCommandDelegate();
	void StartWaitingForExit();
	void Close();
	std::function<void()> exitListener;

	PROCESS_INFORMATION processInfo{};
	HANDLE stdinRead{};
	HANDLE stdinWrite{};

	std::thread waitExitThread;
	//HANDLE stdoutRead;
	//HANDLE stdoutWrite
};

