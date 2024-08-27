#pragma once
#include <Windows.h>
#include <string>
#include "result.h"

namespace InternalCommand
{
	Result<DWORD> Dir(const std::wstring& param, HANDLE out);
	Result<DWORD> Cd(const std::wstring& param, HANDLE out);
	Result<DWORD> Pushd(const std::wstring& param, HANDLE out);
	Result<DWORD> Popd(const std::wstring& param, HANDLE out);
	Result<DWORD> Set(const std::wstring& param, HANDLE out);
	DWORD Echo(const std::wstring& param, HANDLE out);
	Result<DWORD> More(const std::wstring& param, HANDLE out);
	Result<DWORD> Cls(HANDLE out);
	Result<DWORD> Type(const std::wstring& param, HANDLE out);
	bool IsDriveLetter(const std::wstring& str);

};

