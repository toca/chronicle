#pragma once
#include <Windows.h>
#include "Result.h"

namespace ConsoleUtil
{
	/*
	* 
	*/
	Result<CONSOLE_SCREEN_BUFFER_INFOEX> GetConsoleScreenBufferInfo();

	/*
	* 
	*/
	Result<COORD> GetWindowSize();

	/*
	* 
	*/
	Result<COORD> GetConsoleScreenBufferSize();

	/*
	* Caluculate distans from 2 coordinate in console screen.
	*/
	Result<size_t> CalcDistance(const COORD& from, const COORD& to);
	/*
	* Caluculate coordinate
	*/
	Result<COORD> CalcCoord(const COORD& origin, int scalar);
};

