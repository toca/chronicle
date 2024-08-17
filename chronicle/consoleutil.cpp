#include "consoleutil.h"
#include <Windows.h>
#include "stringutil.h"
#include "result.h"

namespace ConsoleUtil
{
	Result<CONSOLE_SCREEN_BUFFER_INFOEX> GetConsoleScreenBufferInfo()
	{
		auto stdoutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		return GetConsoleScreenBufferInfo(stdoutHandle);
	}


	Result<CONSOLE_SCREEN_BUFFER_INFOEX> GetConsoleScreenBufferInfo(HANDLE handle)
	{
		if (handle == INVALID_HANDLE_VALUE) {
			auto err = ::GetLastError();
			return { std::nullopt, Error(err, L"Failed to ::GetStdHandle") };
		}
		CONSOLE_SCREEN_BUFFER_INFOEX info{};
		info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
		if (!::GetConsoleScreenBufferInfoEx(handle, &info)) {
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


	Result<COORD> GetWindowSize(HANDLE handle)
	{
		auto [info, err] = GetConsoleScreenBufferInfo(handle);
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


	OptionalError WriteConsoleOutputEx(HANDLE console, const std::wstring& text, WORD attributes, COORD startCoord) 
	{
		// console screen info
		CONSOLE_SCREEN_BUFFER_INFOEX info{};
		info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
		if (!::GetConsoleScreenBufferInfoEx(console, &info)) {
			auto err = ::GetLastError();
			return Error(err, L"Failed to ::GetConsoleScreenBufferInfoEx@consoleutil");
		}
		int windowWidth = info.srWindow.Right - info.srWindow.Left + 1;

		// Display width for text
		auto [displayWidths, err] = StringUtil::FetchDisplayWidth(text);

		// 
		int x = startCoord.X;
		int y = startCoord.Y;

		// Output each line
		for (size_t i = 0; i < text.length(); ) {
			std::vector<CHAR_INFO> charInfos;
			int lineWidth = windowWidth - x;
			int consumedWidth = 0;

			while (i < text.length() && consumedWidth < lineWidth) {
				consumedWidth += displayWidths->at(i);
				// Full-width on a edge
				if (lineWidth < consumedWidth) {
					consumedWidth--;
					break; // go to new line
				}
				else {
					CHAR_INFO charInfo{};
					charInfo.Char.UnicodeChar = text[i];
					charInfo.Attributes = attributes;
					charInfos.push_back(charInfo);
				}

				// If character is full-width append space
				if (displayWidths->at(i) == 2) {
					CHAR_INFO charInfo{};
					charInfo.Char.UnicodeChar = L' ';
					charInfo.Attributes = attributes;
					charInfos.push_back(charInfo);
				}
				i++;
			}

			// range for writing
			COORD bufferSize = { SHORT(charInfos.size()), 1 };
			COORD bufferCoord = { 0, 0 };
			SMALL_RECT writeRegion = { x, y, SHORT(x + consumedWidth - 1), y };

			// write to console
			WriteConsoleOutputW(console, charInfos.data(), bufferSize, bufferCoord, &writeRegion);

			// go to new line
			x = 0;
			y++;
		}
		return std::nullopt;
	}
}
