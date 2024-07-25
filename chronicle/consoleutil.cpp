#include "consoleutil.h"
#include <Windows.h>
#include "result.h"

namespace ConsoleUtil
{
	Result<CONSOLE_SCREEN_BUFFER_INFOEX> GetConsoleScreenBufferInfo()
	{
		auto stdoutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		if (stdoutHandle == INVALID_HANDLE_VALUE) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, L"Failed to ::GetStdHandle") };
		}
		CONSOLE_SCREEN_BUFFER_INFOEX info{};
		info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
		if (!::GetConsoleScreenBufferInfoEx(stdoutHandle, &info)) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, L"Failed to ::GetConsoleScreenBufferInfoEx") };
		}
		return { info, std::nullopt };
	}


	Result<COORD> GetWindowSize()
	{
		auto [info, err] = GetConsoleScreenBufferInfo();
		if (err) {
			return { std::nullopt, err };
		}
		return { COORD { SHORT(info->srWindow.Right - info->srWindow.Left + 1), SHORT(info->srWindow.Bottom - info->srWindow.Top + 1) }, std::nullopt };
	}


	Result<COORD> GetConsoleScreenBufferSize()
	{
		auto [info, err] = GetConsoleScreenBufferInfo();
		if (err) {
			return { std::nullopt, err };
		}
		return { COORD { info->dwSize.X, info->dwSize.Y }, std::nullopt };
	}

	/*
	* Caluculate distans from 2 coordinate in console screen.
	*/
	Result<size_t> CalcDistance(const COORD& from, const COORD& to)
	{
		auto [info, err] = GetConsoleScreenBufferInfo();
		if (err) {
			return { std::nullopt, err };
		}
		auto width = info->srWindow.Right - info->srWindow.Left + 1;
		auto dy = int64_t(to.Y - from.Y);
		auto dx = int64_t(to.X - from.X);
		return { dy * width + dx, std::nullopt };
	}


	/*
	* Caluculate coordinate
	*/
	Result<COORD> CalcCoord(const COORD& origin, int scalar)
	{
		auto [info, err] = GetConsoleScreenBufferInfo();
		if (err) {
			return { std::nullopt, err };
		}
		auto width = info->srWindow.Right - info->srWindow.Left + 1;
		int dy = (scalar + origin.X) / width;
		int x = (scalar + origin.X) % width;
		return { COORD{ SHORT(x), SHORT(origin.Y + dy) }, std::nullopt };
	}

}
