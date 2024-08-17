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
class Candidate;


class Controller
{
public:
	static Result<Controller*> Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history);
	~Controller();
	OptionalError Input(const std::vector<INPUT_RECORD>& inputs);
	OptionalError Render();
	void OnModeChanged(Mode mode);
private:
	Controller(std::shared_ptr<View> view, std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history, std::shared_ptr<Candidate> candidate);
	OptionalError KeyEvent(const KEY_EVENT_RECORD& keyEvent);
	void Up();
	void Down();
	void Enter();
	void Clear();
	void Complement(bool reverse);
	void Commit();

	std::shared_ptr<View> view;
	std::shared_ptr<InputBuffer> inputBuffer;
	std::shared_ptr<History> history;
	std::shared_ptr<Candidate> candidate;
	std::vector<std::wstring> candidates;
	bool complementing = false;
};

