#include "inputbufferwindow.h"
#define NOMINMAX
#include <Windows.h>
#include "inputbuffer.h"
#include "stringutil.h"

InputBufferWindow::InputBufferWindow(std::shared_ptr<InputBuffer> inputBuffer)
	: inputBuffer(inputBuffer)
	, width(0)
	, left(0)
	, right(0)
{
}


InputBufferWindow::~InputBufferWindow()
{
}


void InputBufferWindow::SetWidth(size_t width)
{
	this->width = width - 1; // Leave one character space at the left edge of the screen
	this->right = width - 2; // Size to index + space
}


bool InputBufferWindow::ConsumeUpdatedFlag()
{
	return this->inputBuffer->ConsumeUpdatedFlag();
}


std::wstring InputBufferWindow::Get()
{
	auto cursor = this->inputBuffer->GetCursor();
	// move to right
	if (this->right < cursor) {
		this->right = cursor;
		this->left = this->right - this->width;
	}
	// move to left
	if (cursor < this->left) {
		this->left = cursor;
		this->right = this->left + this->width;
	}

	std::wstring input = this->inputBuffer->Get();
	auto [widths, err] = StringUtil::FetchDisplayWidth(input);
	if (err) {
		// FIXME
		return L"error@inputbufferwindow";
	}

	// left side
	size_t startPos = 0;
	this->droppedWidth = 0;
	int remainWidth = this->left;
	for (int i = 0; 0 < remainWidth && i < widths->size(); i++) {
		startPos++;
		remainWidth -= widths->at(i);
		this->droppedWidth += widths->at(i);
	}

	// right side
	size_t endPos = startPos;
	remainWidth = this->right - this->left;
	for (int i = startPos; 0 < remainWidth && i < widths->size(); i++) {
		endPos++;
		remainWidth -= widths->at(i);
	}
	// over window left
	if (remainWidth < 0) {
		endPos--;
	}
	return std::wstring(input.begin() + startPos, input.begin() + endPos);
}



SHORT InputBufferWindow::GetCursor()
{
	return this->inputBuffer->GetCursor() - this->droppedWidth;
}
