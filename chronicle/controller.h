#pragma once
#include <Windows.h>
#include <vector>
#include <memory>
#include "error.h"

class InputBuffer;
class PromptGate;
class History;

class Controller
{
public:
	Controller(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<PromptGate> promptGate, std::shared_ptr<History> history);
	~Controller();

	OptionalError Input(const std::vector<INPUT_RECORD>& inputs);
private:
	void Up();
	void Down();
	void Enter();

	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<PromptGate> promptGate;
	std::shared_ptr<History> history;
};

