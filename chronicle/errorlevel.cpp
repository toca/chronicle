#include "errorlevel.h"

static DWORD errorLevel = 0;

DWORD GetErrorLevelAsCode()
{
	return errorLevel;
}

void SetErrorLevel(DWORD code)
{
	errorLevel = code;
}

std::wstring GetErrorLevel()
{
	return std::to_wstring(errorLevel);
}
