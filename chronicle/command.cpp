#include "command.h"
#include <Windows.h>
#include <process.h>
#include <cstdio>

#include "error.h"
#include "result.h"

namespace Command 
{
	int Execute(const std::string& command)
	{
		int result = system(command.c_str());
		puts("");
		auto r = ::SetEnvironmentVariableA("ERRORLEVEL", std::to_string(result).c_str());
		return result;
	}	


	//size_t CalcDistance(const SMALL_RECT& windowRect, const COORD& from, const COORD& to)
	//{
	//	auto width = windowRect.Right - windowRect.Left + 1;
	//	auto y = int64_t(to.Y - from.Y * width);
	//	auto x = int64_t(to.X - from.X);
	//	return (to.Y - from.Y) * width + to.X - from.X;
	//}
	
}
