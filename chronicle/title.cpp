#include "title.h"
#include <Windows.h>
#include <string>
#include "error.h"

namespace Title
{
	OptionalError Set(const std::wstring& title)
	{
		if (::SetConsoleTitleW(title.c_str())) {
			return std::nullopt;
		}
		else {
			return Error(::GetLastError(), L"Failed to SetConsoleTitle Set@title");
		}
	}
}
