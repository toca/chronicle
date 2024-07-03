#include "controller.h"
#include "inputbuffer.h"
#include "command.h"
#include "promptgate.h"

Controller::Controller(std::shared_ptr<InputBuffer> ib, std::shared_ptr<PromptGate> pg)
	: inputBuffer(ib)
	, promptGate(pg)
{
}

Controller::~Controller()
{
}

OptionalError Controller::Input(const std::vector<INPUT_RECORD>& inputs)
{

	for (auto& each : inputs) {
		switch (each.EventType) {
		case KEY_EVENT:
			this->inputBuffer->InputKey(each.Event.KeyEvent);
			if (each.Event.KeyEvent.bKeyDown && each.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
				Command::Execute(inputBuffer->GetCommand());
				this->inputBuffer->ClearInput();
				this->promptGate->GetReady();
			}
			break;
		case MOUSE_EVENT:
			break;
		case WINDOW_BUFFER_SIZE_EVENT:
			break;
		case MENU_EVENT:
			break;
		case FOCUS_EVENT:
			break;
		default:
			break;
		}
	}
	return OptionalError();
}
