#pragma once

#include <Windows.h>
#include <optional>
#include <vector>
#include "result.h"

class SearchView
{
public:
	~SearchView();
	static Result<SearchView*> Create();
	std::optional<Error> Show();
	void Stop();
	std::tuple<std::vector<INPUT_RECORD>, std::optional<Error>> Read();
	std::optional<Error> Render();
	void ProcessInputs(const std::vector<INPUT_RECORD> inputs);
	//void Write(const std::string& s);
private:
	SearchView();
	std::string prompt{};
	std::string inputting{};
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

