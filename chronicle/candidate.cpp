#include "candidate.h"
#include <vector>
#include <string>
#include <optional>
#include "completionitem.h"

Candidate::Candidate()
{
}


Candidate::~Candidate()
{
}


void Candidate::Set(const std::vector<Completion::Item>& candidates)
{
	this->data = candidates;
	this->index = -1;
}


void Candidate::Reset()
{
	this->index = -1;
	this->updated = true;
}


std::optional<Completion::Item> Candidate::Get()
{
	// nothing to show.
	if (this->index == -1) {
		return Completion::Item{ L"", L"" };
	}

	// failure
	if (this->data.empty()) {
		return std::nullopt;
	}
	return this->data.at(this->index);
}

void Candidate::Forward()
{
	if (this->index < int(this->data.size() - 1)) {
		this->updated = true;
		this->index++;
	}
	else {
		this->index = 0;
		this->updated = true;
	}
}

void Candidate::Back()
{
	if (1 < this->index) {
		this->updated = true;
		this->index--;
	}
	else {
		this->index = this->data.size() - 1;
		this->updated = true;
	}
}


bool Candidate::ConsumeUpdateFlag()
{
	bool result = this->updated;
	this->updated = false;
	return result;
}
