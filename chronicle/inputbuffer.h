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
	std::wstring Get();
	std::wstring GetCommand();
	void Set(const std::wstring& s);
	SHORT GetCursor();
	void ClearInput();
	void SetOnChange(std::function<void(InputBuffer*)> callback);

private:
	std::vector<wchar_t> buffer{};
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
	void Clear();
};

