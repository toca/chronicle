#pragma once
#include <Windows.h>
#include <string>
#include <optional>

struct Error {
	DWORD code;
	const std::string message;
};

using OptionalError = std::optional<Error>;