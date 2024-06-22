#pragma once

#include <Windows.h>
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include "result.h"

class SearchController;
class Prompt;
class Historian;

class SearchView
{
public:
	SearchView();
	~SearchView();
	Result<std::string> Show(const std::vector<std::string>& histories);
	void Stop();
	std::tuple<std::vector<INPUT_RECORD>, std::optional<Error>> Read();
	std::optional<Error> Render();
private:
	std::optional<Error> Init(const std::vector<std::string>& histories);

	// Controller
	std::unique_ptr<SearchController> controller;
	// Models
	std::shared_ptr<Prompt> prompt;
	std::shared_ptr<Historian> historian;

	std::optional<std::string> result = std::nullopt;
	COORD size;
	HANDLE stdOutHandle{};
	HANDLE stdInHandle{};
	HANDLE screenBuffers[2];
	COORD windowSize{};
	COORD cursorPos{};
	int32_t screenIndex = 0;
	uint32_t row = 0;
	uint32_t col = 0;

	bool stop = false;
	bool updated = false;

};

