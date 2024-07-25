#include "controller.h"
#include "inputbuffer.h"
#include "command.h"
#include "promptgate.h"
#include "history.h"

Controller::Controller(std::shared_ptr<InputBuffer> ib, std::shared_ptr<PromptGate> pg, std::shared_ptr<History> hist)
	: inputBuffer(ib)
	, promptGate(pg)
	, history(hist)
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
			// pass through all inputs to InputBuffer
			this->inputBuffer->InputKey(each.Event.KeyEvent);
			// on key down
			if (each.Event.KeyEvent.bKeyDown)
			{
				if (each.Event.KeyEvent.wVirtualKeyCode == VK_UP) {
					this->Up();
				}
				else if (each.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
					this->Down();
				}
				else if (each.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
					this->Enter();
				}
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

void Controller::Up()
{
	auto s = history->Older();
	if (s) {
		this->inputBuffer->ClearInput();
		this->inputBuffer->Set(*s);
	}
}

void Controller::Down()
{
	auto s = history->Newer();
	if (s) {
		this->inputBuffer->ClearInput();
		this->inputBuffer->Set(*s);
	}
}

void Controller::Enter()
{
 	printf("\n");
	const std::wstring command =  inputBuffer->GetCommand();
	if (!command.empty()) {
		auto [code, err] = Command::Execute(command);
		if (err) {
			fwprintf(stderr, L"%s: %d\n", err->message.c_str(), err->code);
			// show error in Command or View
		}
		this->inputBuffer->ClearInput();
		printf("\n");
	}
	this->promptGate->GetReady();
}

