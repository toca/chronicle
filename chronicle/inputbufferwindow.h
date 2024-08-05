#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include <functional>
#include "error.h"

// Proto
class InputBuffer;

// This class represents the display range of the input buffer.
// And scroll based on the cursor position.
//
// ==== input buffer =====
// a b c d e f g h i j k
// _ _ _ _ _ _ _ _ _ _ _
//    ^ --- left    ^
//                  `--- right

// Input Buffer Wrapper for SearchView
class InputBufferWindow
{
public:
	InputBufferWindow(std::shared_ptr<InputBuffer> inputBuffer);
	~InputBufferWindow();

	void SetWidth(size_t width);
	bool ConsumeUpdatedFlag();

	std::wstring Get();
	SHORT GetCursor();

private:
	std::shared_ptr<InputBuffer> inputBuffer;
	size_t width;
	// offset in display width
	size_t left;
	// offset in display width
	size_t right;
	size_t droppedWidth = 0;
	//bool odd = false;
};

