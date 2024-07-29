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


std::optional<Error> History::Load(std::wistream& stream)
{
	// stream ----
	// newest <- end - 0
	// 2nd    <- end - 1
	// 3rd    <- end - 2
	int count = 0;
	std::wstring line;
	while (std::getline(stream, line) && count < DataLength) {
		this->data.push_front(line);
		count++;
	}
	this->Reset();
	return std::nullopt;
}


std::optional<Error> History::Dump(std::wostream& stream) 
{
	for (auto it = this->data.rbegin(); it != this->data.rend(); it++) {
		stream << *it << std::endl;
	}
	return std::nullopt;
}


void History::Add(const std::wstring& line)
{
	if (line.size()) {
		this->data.remove(line);
		this->data.push_back(line);
	}
	this->Reset();
}


std::optional<std::wstring> History::Newer()
{
	if (this->data.empty()) {
		return std::nullopt;
	}
	if (current == this->data.end()) {
		return std::nullopt;
	}
	
	++current;
	if (current == this->data.end()) {
		--current;
		return std::nullopt;
	}
	else {
		return *current;
	}
}


std::optional<std::wstring> History::Older()
{
	if (this->data.empty()) {
		return std::nullopt;
	}
	if (current != this->data.begin()) {
		--current;
	}
	else {
		return std::nullopt;
	}
	return *current;
}


void History::Reset() {
	this->current = this->data.end();
}


std::vector<std::wstring> History::GetAll()
{
	return std::vector<std::wstring>(this->data.begin(), this->data.end());
}
