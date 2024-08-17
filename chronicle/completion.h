#pragma once
#include <vector>
#include <string>
#include <filesystem>

namespace Completion
{
	struct Item;

	std::vector<Item> Candidates(const std::wstring& input, const std::wstring& currentDirectory);
};

