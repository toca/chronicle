#pragma once
#include <Windows.h>
#include <string>
#include "result.h"

namespace InternalCommand
{
	Result<DWORD> Dir(const std::string& param, HANDLE out);
	Result<DWORD> Cd(const std::string& param, HANDLE out);
	Result<DWORD> Pushd(const std::string& param, HANDLE out);
	Result<DWORD> Popd(const std::string& param, HANDLE out);
	Result<DWORD> Set(const std::string& param, HANDLE out);
	DWORD Echo(const std::string& param, HANDLE out);
	bool IsDriveLetter(const std::string& str);

};

