#pragma once
#include <memory>
#include "error.h"
#include "result.h"

// prototype
class InputBuffer;

class View
{
public:
	~View();

	static Result<View*> Create(std::shared_ptr<InputBuffer> inputBuffer);
	OptionalError Render();
	void ShowPrompt();
	void SetTitle();
	void Renew();
	void Enable(bool state);
	
private:
	View(std::shared_ptr<InputBuffer> inputBuffer);
	void ShowInputBuffer();

	// models
	std::shared_ptr<InputBuffer> inputBuffer;

	bool enabled = false;
	HANDLE stdOutHandle{};
	HANDLE stdInHandle{};
	COORD cursorOrigin{};

};

