#include "inputbufferwindow.h"
#define NOMINMAX
#include <Windows.h>
#include "inputbuffer.h"

InputBufferWindow::InputBufferWindow(std::shared_ptr<InputBuffer> inputBuffer)
	: inputBuffer(inputBuffer)
	, callback(nullptr)
	, maxCol(0)
	, left(0)
	, right(0)
{
	this->inputBuffer->SetOnChange([this](auto ib)
		{
			this->OnChanged(ib);
		}
	);
}


InputBufferWindow::~InputBufferWindow()
{
}


void InputBufferWindow::SetMaxCol(size_t col)
{
	// FIXME
	this->maxCol = col;
	this->left = 0;
	this->right = col;
}


OptionalError InputBufferWindow::InputKey(const KEY_EVENT_RECORD& e)
{
	return this->inputBuffer->InputKey(e);
}


std::wstring InputBufferWindow::Get()
{
	// TODO use display count
	auto input = this->inputBuffer->Get();
	return std::wstring(input.begin() + left, input.begin() + (std::min)(right, input.size()));
	//return this->inputBuffer->Get();
}


void InputBufferWindow::Set(const std::wstring& s)
{
	this->inputBuffer->Set(s);
}


SHORT InputBufferWindow::GetCursor()
{
	return this->inputBuffer->GetCursor() - this->left;
}


void InputBufferWindow::ClearInput()
{
	this->inputBuffer->ClearInput();
}


void InputBufferWindow::SetOnChange(std::function<void(InputBuffer*)> callback)
{
	this->callback = callback;
}


void InputBufferWindow::OnChanged(InputBuffer* ib)
{
	auto cursor = this->inputBuffer->GetCursor();
	if (this->right < cursor) {
		this->right = cursor;
		this->left = this->right - this->maxCol;
	}
	if (cursor < this->left) {
		this->left = cursor;
		this->right = this->left + this->maxCol;
	}

	if (this->callback) {
		this->callback(ib);
	}
}

