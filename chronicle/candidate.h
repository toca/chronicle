#pragma once
#include <vector>
#include <string>
#include <optional>
#include "completionitem.h"

class Candidate
{
public:
	Candidate();
	~Candidate();
	void Set(const std::vector<Completion::Item>& candidates);
	void Reset();
	std::optional<Completion::Item> Get();
	void Forward();
	void Back();
	bool ConsumeUpdateFlag();
private:
	std::vector<Completion::Item> data{};
	int index = -1;
	bool updated = false;
};

