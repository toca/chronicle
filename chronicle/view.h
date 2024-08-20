#pragma once
#include <memory>
#include "error.h"
#include "result.h"

// prototype
class InputBuffer;
class Candidate;

class View
{
public:
	~View();

	static Result<View*> Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<Candidate> candidate);
	OptionalError Render();
	void ShowPrompt();
	void SetTitle();
	void Renew();
	void Clear();
	void Enable(bool state);
	
private:
	View(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<Candidate> candidate);
	OptionalError ShowInputBuffer();
	OptionalError ShowCandidate();

	// models
	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<Candidate> candidate;

	bool enabled = false;
	HANDLE stdOutHandle{};
	HANDLE stdInHandle{};
	// Pos in screen buffer
	COORD cursorOrigin{};

};

