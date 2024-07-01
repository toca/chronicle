#pragma once
#include <Windows.h>
#include <vector>
#include <memory>
#include "error.h"

class InputBuffer;
class PromptGate;

class Controller
{
public:
	Controller(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<PromptGate> promptGate);
	~Controller();

	OptionalError Input(const std::vector<INPUT_RECORD>& inputs);
private:
	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<PromptGate> promptGate;
};

