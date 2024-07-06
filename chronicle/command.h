#pragma once
#include <optional>
#include <string>
#include "error.h"

namespace Command 
{
	OptionalError Execute(const std::string& command);
}
