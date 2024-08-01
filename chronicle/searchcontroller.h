#pragma once
#include <Windows.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include "result.h"
#include "mode.h"

class SearchView;
class InputBuffer;
class Historian;
class History;

class SearchController
{
public:
	static Result<SearchController*> Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history);
	~SearchController();
	void Input(const std::vector<INPUT_RECORD>& inputs);
	void OnModeChanged(Mode mode);
	void OnWindowSizeEvent();
private:
	SearchController(std::shared_ptr<SearchView> view, std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<Historian> historian, std::shared_ptr<History> history);

	std::shared_ptr<SearchView> view;
	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<Historian> historian;
	std::shared_ptr<History> history;
	std::wstring inputting = L"";
	COORD windowSize{};

};

