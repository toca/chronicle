#pragma once
#include <vector>
#include <optional>
#include <string>
#include <functional>

struct Item {
	int index;
	std::wstring data;
	bool selected;
};

class Historian
{
public:
	Historian();
	~Historian();
	void SetData(const std::vector<std::wstring>& histories);
	void SetMaxRowCount(size_t maxRowCount);
	void SetOnChanged(std::function<void()> callback);
	std::optional<Item> At(int index);
	void Filter(const std::wstring& keyword);
	void Next();
	void Prev();
	std::optional<std::wstring> Current();
	int Top();
	int Bottom();

private:
	const std::vector<std::wstring>& Data();
	std::function<void()> callback;
	std::vector<std::wstring> data;
	std::vector<std::wstring> candidates;
	int index;
	int top;
	int bottom;
	size_t rowCount;
	void Updated();

};

