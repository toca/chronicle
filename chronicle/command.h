#pragma once
#include <optional>
#include <string>
#include "error.h"

namespace Command 
{
	DWORD Execute(const std::string& command);
}
