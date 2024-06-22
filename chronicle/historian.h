#pragma once
#include <vector>
#include <optional>
#include <string>

struct Item {
	int index;
	std::string data;
	bool selected;
};

class Historian
{
public:
	Historian(const std::vector<std::string>& histories, size_t maxRowCount);
	~Historian();
	std::optional<Item> At(int index);
	void Filter(const std::string& keyword);
	void Next();
	void Prev();
	std::optional<std::string> Current();
	int Top();
	int Bottom();
	bool NeedUpdate();
	void ResetUpdateStatus();

private:
	const std::vector<std::string>& Data();
	const std::vector<std::string>& data;
	std::vector<std::string> candidates;
	int index;
	int top;
	int bottom;
	size_t rowCount;
	bool updated = true;
};

