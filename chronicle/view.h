#pragma once
#include <memory>
#include "error.h"
#include "result.h"

// prototype
class InputBuffer;
class PromptGate;

class View
{
public:
	static Result<View*> Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<PromptGate> promptGate);
	~View();
	
	OptionalError Render();
private:
	View(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<PromptGate> promptGate);
	void ShowInputBuffer();
	void ShowPrompt();

	// models
	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<PromptGate> promptGate;

	//COORD size{ 0, 0 };
	HANDLE stdOutHandle{};
	HANDLE stdInHandle{};
	COORD cursorOrigin{};
	//HANDLE screenBuffers[2];
	//COORD windowSize{};
	//int32_t screenIndex = 0;
	//uint32_t row = 0;
	//uint32_t col = 0;

	bool stop = false;
	bool updated = false;
};

