#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>

// TODO Remove
class Prompt
{
public:
	Prompt();
	~Prompt();
	std::wstring Get();
	std::wstring GetRawStr();
	SHORT GetCursor();
	void InputKey(const KEY_EVENT_RECORD& keyEvent);
	void Clear();
	void SetOnChanged(std::function<void()> callback);
private:
	std::vector<wchar_t> buffer;
	std::function<void()> callback;
	SHORT cursorIndex = 0;
	void Updated();
	void Left();
	void Right();
};

