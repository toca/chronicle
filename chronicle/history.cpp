#include "history.h"
#include <fstream>
#include <iostream>

History::History()
{
	this->Reset();
}

History::~History()
{
}


std::optional<Error> History::Load(std::ifstream& stream)
{
	int count = 0;
	std::string line;
	while (std::getline(stream, line) && count < DataLength) {
		this->data.push_front(line);
		count++;
	}
	return std::nullopt;
}


std::optional<Error> History::Dump(std::ofstream& stream) 
{
	for (auto& each : this->data) {
		stream << each << std::endl;
	}
	return std::nullopt;
}


void History::Add(const std::string& line)
{
	if (line.size()) {
		this->data.remove(line);
		this->data.push_back(line);
	}
	this->Reset();
}

std::optional<std::string> History::Prev()
{
	if (this->data.empty()) {
		return std::nullopt;
	}
	if (current != this->data.begin()) {
		--current;
	}
	return *current;
}

std::optional<std::string> History::Next()
{
	if (this->data.empty()) {
		return std::nullopt;
	}
	if (current != this->data.end()) {
		++current;
		if (current == this->data.end()) {
			--current;
		}
	}
	else {
		return std::nullopt;
	}
	return *current;
}


void History::Reset() {
	this->current = this->data.end();
}
