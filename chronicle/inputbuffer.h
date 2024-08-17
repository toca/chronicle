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
	std::wstring GetInputToCursor();
	void Set(const std::wstring& s);
	void Append(const std::wstring& s);
	SHORT GetCursor();
	void ClearInput();
	bool ConsumeUpdatedFlag();
	bool PeekUpdatedFlag();

private:
	std::vector<wchar_t> buffer{};
	SHORT cursorIndex = 0;
	bool updated = false;

	void OnChanged();
	void Control(WORD vk);

	void Left();
	void Right();
	void Back();
	void Del();
	void Home();
	void End();
	void Clear();
};

