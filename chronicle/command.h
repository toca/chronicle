#pragma once
#include <optional>
#include <string>
#include "result.h"
#include "error.h"

namespace Command 
{
	Result<DWORD> Execute(const std::string& command);
}
