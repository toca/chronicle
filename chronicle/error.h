#pragma once
#include <Windows.h>
#include <string>
#include <optional>
#include <format>

struct Error {
	DWORD code;
	const std::wstring message;

    Error(int code, const std::wstring& message)
        : code(code)
        , message(message) 
    {}

    Error(const Error& other, const std::wstring& message)
        : code(other.code)
        , message(std::format(L"%s\n\t%s", message, other.message))
    {}
};

using OptionalError = std::optional<Error>;