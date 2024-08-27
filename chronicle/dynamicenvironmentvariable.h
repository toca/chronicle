#pragma once
#include <string>
#include "result.h"

namespace Command
{
	Result<std::wstring> ExpandDynamicEnv(const std::wstring& name);
}
