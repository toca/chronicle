#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <functional>
#include "error.h"

class InputBuffer
{
public:
	InputBuffer();
	~InputBuffer();

	OptionalError InputKey(const KEY_EVENT_RECORD& e);
	std::string Get();
	SHORT GetCursor();
	void ClearInput();
	void SetOnChange(std::function<void(InputBuffer*)> callback);

private:
	std::vector<char> buffer{};
	SHORT cursorIndex = 0;
	std::function<void(InputBuffer*)> callback;
	bool updated = false;

	void OnChanged();

	void Left();
	void Right();
	void Back();
	void Del();
	void Home();
	void End();
};

