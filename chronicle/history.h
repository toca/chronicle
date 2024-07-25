#pragma once

#include <string>
#include <optional>
#include <list>
#include <vector>
#include "error.h"

constexpr uint32_t DataLength = 1024;

class History
{
public:
	History();
	~History();
	std::optional<Error> Load(std::wistream& stream);
	std::optional<Error> Dump(std::wostream& stream);
	void Add(const std::wstring& line);
	std::optional<std::wstring> Older();
	std::optional<std::wstring> Newer();
	void Reset();
	std::vector<std::wstring> GetAll();
private:
	std::list<std::wstring> data;
	std::list<std::wstring>::iterator current;
	uint32_t index = 0;

};

