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
	std::optional<Error> Load(std::istream& stream);
	std::optional<Error> Dump(std::ostream& stream);
	void Add(const std::string& line);
	std::optional<std::string> Older();
	std::optional<std::string> Newer();
	void Reset();
	std::vector<std::string> GetAll();
private:
	std::list<std::string> data;
	std::list<std::string>::iterator current;
	uint32_t index = 0;

};

