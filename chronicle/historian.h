#pragma once
#include <vector>
#include <optional>
#include <string>

struct Item {
	int index;
	std::wstring data;
	bool selected;
};

class Historian
{
public:
	Historian(const std::vector<std::wstring>& histories, size_t maxRowCount);
	~Historian();
	std::optional<Item> At(int index);
	void Filter(const std::wstring& keyword);
	void Next();
	void Prev();
	std::optional<std::wstring> Current();
	int Top();
	int Bottom();
	bool NeedUpdate();
	void ResetUpdateStatus();

private:
	const std::vector<std::wstring>& Data();
	const std::vector<std::wstring>& data;
	std::vector<std::wstring> candidates;
	int index;
	int top;
	int bottom;
	size_t rowCount;
	bool updated = true;
};

