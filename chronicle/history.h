#pragma once

#include <string>
#include <optional>
#include <list>
#include <vector>
#include <iostream>
#include "error.h"

constexpr uint32_t DataLength = 1024;

class History
{
public:
	History(std::wifstream&& in, std::wofstream&& out);
	~History();
	void Add(const std::wstring& line);
	std::optional<std::wstring> Older();
	std::optional<std::wstring> Newer();
	void Reset();
	std::vector<std::wstring> GetAll();
private:
	std::optional<Error> Load(std::wistream& stream);
	std::list<std::wstring> data;
	std::list<std::wstring>::iterator current;
	uint32_t index = 0;
	std::wifstream& input;
	std::wofstream& output;

};

