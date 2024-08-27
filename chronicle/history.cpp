#include "history.h"
#include <fstream>
#include <iostream>

History::History(std::wifstream&& in, std::wofstream&& out)
	: input(in)
	, output(out)
{
	this->Load(this->input);
}

History::~History()
{
	this->input.close();
	this->output.close();
}


std::optional<Error> History::Load(std::wistream& stream)
{
	// Data order of stream ----
	// oldest -> begin()
	// ......
	// 3rd    -> end() - 2
	// 2nd    -> end() - 1
	// newest -> end() - 0
	int count = 0;
	std::wstring line;
	while (std::getline(stream, line) && count < DataLength) {
		this->data.push_back(line);
		count++;
	}
	this->Reset();
	return std::nullopt;
}


void History::Add(const std::wstring& line)
{
	if (line.size()) {
		bool removed = this->data.remove(line);
		this->data.push_back(line);
		if (!removed) {
			this->output << line << std::endl;
			this->output.flush();
		}
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
