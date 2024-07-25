#pragma once
#include <Windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>

class Prompt;
class Historian;

class SearchController
{
public:
	SearchController(std::shared_ptr<Prompt> prompt, std::shared_ptr<Historian> historian);
	~SearchController();
	void Input(const std::vector<INPUT_RECORD>& inputs);
	void OnCancel(std::function<void()> callback);
	void OnCompleted(std::function<void(const std::wstring&)> callback);
private:
	std::shared_ptr<Prompt> prompt;
	std::shared_ptr<Historian> historian;
	std::function<void()> cancelCallback;
	std::function<void(const std::wstring&)> completedCallback;
	COORD windowSize{};
};

