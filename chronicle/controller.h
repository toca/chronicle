#pragma once
#include <Windows.h>
#include <vector>
#include <memory>
#include "mode.h"
#include "result.h"
#include "error.h"

class View;
class InputBuffer;
class History;

class Controller
{
public:
	static Result<Controller*> Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history);
	~Controller();
	OptionalError Input(const std::vector<INPUT_RECORD>& inputs);
	void OnModeChanged(Mode mode);
private:
	Controller(std::shared_ptr<View> view, std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history);
	void Up();
	void Down();
	void Enter();

	std::shared_ptr<View> view;
	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<History> history;
};

