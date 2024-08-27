#pragma once

#include <Windows.h>
#include <string>

DWORD GetErrorLevelAsCode();

void SetErrorLevel(DWORD code);

std::wstring GetErrorLevel();

