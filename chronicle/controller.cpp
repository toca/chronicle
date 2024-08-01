#include "controller.h"
#include "view.h"
#include "inputbuffer.h"
#include "inputbufferwindow.h"
#include "command.h"
#include "history.h"
#include "mode.h"

Controller::Controller(
	std::shared_ptr<View> v,
	std::shared_ptr<InputBuffer> ib,
	std::shared_ptr<History> hi
)
	: view(v)
	, inputBuffer(ib)
	, history(hi)
{
	this->view->Enable(true);
	this->view->SetTitle();
	view->ShowPrompt();
}


Result<Controller*> Controller::Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history)
{
	auto [res, err] = View::Create(inputBuffer);
	if (err) {
		return { std::nullopt, err };
	}
	Controller* self = new Controller(std::shared_ptr<View>(*res), inputBuffer, history);
	return { self, std::nullopt };
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
				else if (
					each.Event.KeyEvent.wVirtualKeyCode == VkKeyScanA('r') &&
					(each.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED || each.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED)
				) {
					SetMode(Mode::Search);
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

void Controller::OnModeChanged(Mode mode)
{
	if (mode == Mode::Main) {
		this->view->SetTitle();
		this->view->Enable(true);
	}
	else {
		this->view->Enable(false);
	}
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
	const std::wstring command = inputBuffer->Get();
	this->view->Renew();
 	//wprintf(L"%s\n", command.c_str());
	if (!command.empty()) {
		this->history->Add(command);
		auto [code, err] = Command::Execute(command);
		if (err) {
			fwprintf(stderr, L"%s: %d\n", err->message.c_str(), err->code);
			// show error in Command or View?
		}
		this->view->Renew();
		this->inputBuffer->ClearInput();
	}
	this->view->ShowPrompt();
}

