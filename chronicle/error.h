#pragma once
#include <Windows.h>
#include <string>
struct Error {
	DWORD code;
	const std::string message;
};