#include "controller.h"
#include "view.h"
#include "inputbuffer.h"
#include "inputbufferwindow.h"
#include "command.h"
#include "history.h"
#include "mode.h"
#include "completion.h"
#include "candidate.h"


Controller::Controller(
	std::shared_ptr<View> v,
	std::shared_ptr<InputBuffer> ib,
	std::shared_ptr<History> hi,
	std::shared_ptr<Candidate> c
)
	: view(v)
	, inputBuffer(ib)
	, history(hi)
	, candidate(c)
{
	this->view->Enable(true);
	this->view->SetTitle();
	view->ShowPrompt();
}


Result<Controller*> Controller::Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history)
{
	std::shared_ptr<Candidate> candidate = std::make_shared<Candidate>();
	auto [res, err] = View::Create(inputBuffer, candidate);
	if (err) {
		return { std::nullopt, err };
	}
	Controller* self = new Controller(std::shared_ptr<View>(*res), inputBuffer, history, candidate);
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
			this->KeyEvent(each.Event.KeyEvent);
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
	return std::nullopt;
}


OptionalError Controller::Render()
{
	return this->view->Render();
}


void Controller::OnModeChanged(Mode mode)
{
	if (mode == Mode::Main) {
		this->view->SetTitle();
		this->view->Enable(true);

		wchar_t currentDir[MAX_PATH];
		::GetCurrentDirectoryW(MAX_PATH, currentDir);
		auto result = Completion::Candidates(this->inputBuffer->GetInputToCursor(), currentDir);
		this->candidate->Set(result);
	}
	else {
		this->view->Enable(false);
	}
}


OptionalError Controller::KeyEvent(const KEY_EVENT_RECORD& keyEvent)
{
	// pass through all inputs to InputBuffer
	this->inputBuffer->InputKey(keyEvent);
	// on key down
	if (!keyEvent.bKeyDown)
	{
		return std::nullopt;
	}

	if (keyEvent.wVirtualKeyCode != VK_TAB) {
		this->complementing = false;
	}

	switch (keyEvent.wVirtualKeyCode)
	{
	case VK_UP:
	{
		this->Up();
		break;
	}
	case VK_DOWN:
	{
		this->Down();
		break;
	}
	case VK_RETURN:
	{
		this->Enter();
		break;
	}
	case VK_TAB:
	{
		this->Complement(keyEvent.dwControlKeyState & SHIFT_PRESSED);
		break;
	}
	case VK_ESCAPE:
	{
		this->candidate->Reset();
		break;
	}
	case VK_RIGHT:
		this->Commit();
	default:
	{
		if (keyEvent.wVirtualKeyCode == VkKeyScanA('r'))
		{
			if (keyEvent.dwControlKeyState & LEFT_CTRL_PRESSED || keyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED) {
				SetMode(Mode::Search);
			}
		}
		else if (keyEvent.wVirtualKeyCode == VkKeyScanA('l'))
		{
			if (keyEvent.dwControlKeyState & LEFT_CTRL_PRESSED || keyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED) {
				this->Clear();
			}
		}
		break;
	}
	}
	return std::nullopt;
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


void Controller::Clear()
{
	this->view->Clear();
	this->view->ShowPrompt();
}


void Controller::Complement(bool reverse)
{

	if (!this->complementing) {
		this->complementing = true;
		wchar_t currentDir[MAX_PATH];
		::GetCurrentDirectoryW(MAX_PATH, currentDir);
		auto result = Completion::Candidates(this->inputBuffer->GetInputToCursor(), currentDir);
		this->candidate->Set(result);
	}

	if (reverse) {
		this->candidate->Back();	
	}
	else {
		this->candidate->Forward();
	}
}


void Controller::Commit()
{
	auto result = this->candidate->Get();
	if (!result || result->suggest.empty()) {
		return;
	}
	this->inputBuffer->Append(result->suggest);
}
