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
	~View();

	static Result<View*> Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<PromptGate> promptGate);
	void ShowPrompt();
	
private:
	View(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<PromptGate> promptGate);
	void Render();
	void ShowInputBuffer();

	// models
	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<PromptGate> promptGate;

	HANDLE stdOutHandle{};
	HANDLE stdInHandle{};
	COORD cursorOrigin{};

};

