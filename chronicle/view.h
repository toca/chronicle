#pragma once
#include <memory>
#include "error.h"
#include "result.h"

// prototype
class InputBuffer;
//class PromptGate;

class View
{
public:
	~View();

	static Result<View*> Create(std::shared_ptr<InputBuffer> inputBuffer);
	void ShowPrompt();
	void SetTitle();
	void Renew();
	
private:
	View(std::shared_ptr<InputBuffer> inputBuffer);
	void ShowInputBuffer();

	// models
	std::shared_ptr<InputBuffer> inputBuffer;

	HANDLE stdOutHandle{};
	HANDLE stdInHandle{};
	COORD cursorOrigin{};

};

