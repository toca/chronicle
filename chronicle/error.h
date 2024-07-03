#pragma once
#include <Windows.h>
#include <string>
#include <optional>
#include <format>

struct Error {
	DWORD code;
	const std::string message;

    Error(int code, const std::string& message)
        : code(code)
        , message(message) 
    {}

    Error(const Error& other, const std::string& message)
        : code(other.code)
        , message(std::format("%s\n\t%s", message, other.message))
    {}
};

using OptionalError = std::optional<Error>;