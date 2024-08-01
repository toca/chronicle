#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include <functional>
#include "error.h"

// Proto
class InputBuffer;

// Input Buffer Wrapper for SearchView
class InputBufferWindow
{
public:
	InputBufferWindow(std::shared_ptr<InputBuffer> inputBuffer);
	~InputBufferWindow();

	void SetMaxCol(size_t col);

	OptionalError InputKey(const KEY_EVENT_RECORD& e);
	std::wstring Get();
	void Set(const std::wstring& s);
	SHORT GetCursor();
	void ClearInput();
	void SetOnChange(std::function<void(InputBuffer*)> callback);

private:
	void OnChanged(InputBuffer*);
	std::shared_ptr<InputBuffer> inputBuffer;
	std::function<void(InputBuffer*)> callback;
	size_t maxCol;
	size_t left;
	size_t right;
};

