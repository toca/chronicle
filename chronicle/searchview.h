#pragma once

#include <Windows.h>
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include "result.h"

class SearchController;
class InputBuffer;
class InputBufferWindow;
class Historian;

class SearchView
{
public:
	static Result<SearchView*> Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<Historian> historian);
	~SearchView();
	OptionalError Render();
	void SetTitle();
	void Enable(bool state);
	void OnWindowSizeEvent();


private:
	SearchView();

	// Models
	std::shared_ptr<InputBufferWindow> inputBuffer;
	std::shared_ptr<Historian> historian;

	bool enabled = false;
	HANDLE stdOutHandle{};
	HANDLE screenBuffers[2];
	COORD windowSize{};
	COORD cursorPos{};
	int32_t screenIndex = 0;
	uint32_t row = 0;
	uint32_t col = 0;

};
