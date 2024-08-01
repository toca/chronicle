#pragma once
#include <Windows.h>
#include "Result.h"

namespace ConsoleUtil
{
	
	// Get Screen Buffer Information from STD_OUTPUT_HANDLE
	Result<CONSOLE_SCREEN_BUFFER_INFOEX> GetConsoleScreenBufferInfo();

	// Get Screen Buffer Information from passed handle
	Result<CONSOLE_SCREEN_BUFFER_INFOEX> GetConsoleScreenBufferInfo(HANDLE handle);

	// Get Window Size from STD_OUTPUT_HANDLE
	Result<COORD> GetWindowSize();

	// Get Window Size from passed handle
	Result<COORD> GetWindowSize(HANDLE handle);


	// Get ConsoleScreenBufferSize from STD_OUTPUT_HANDLE
	Result<COORD> GetConsoleScreenBufferSize();

	
	// Calculate distance from 2 coordinate in console screen.
	Result<size_t> CalcDistance(const COORD& from, const COORD& to);
	
	// Calculate coordinate
	Result<COORD> CalcCoord(const COORD& origin, int scalar);
};

