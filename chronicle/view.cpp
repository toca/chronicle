#include "view.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>

#include "consoleutil.h"
#include "stringutil.h"
#include "result.h"
#include "inputbuffer.h"
#include "errorlevel.h"
#include "title.h"


Result<View*> View::Create(std::shared_ptr<InputBuffer> inputBuffer)
{
	View* self = new View(inputBuffer);

	// std handles
	HANDLE oh = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (oh == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, L"Failed to ::GetStdHandle(STD_OUTPUT_HANDLE") };
	}
	self->stdOutHandle = oh;

	HANDLE ih = ::GetStdHandle(STD_INPUT_HANDLE);
	if (ih == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, L"Failed to ::GetStdHandle(STD_INPUT_HANDLE") };
	}
	self->stdInHandle = ih;


	// console screen buffer info
	CONSOLE_SCREEN_BUFFER_INFOEX infoEx{};
	infoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	if (!::GetConsoleScreenBufferInfoEx(self->stdOutHandle, &infoEx)) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, L"Failed to ::GetConsoleScreenBufferInfoEx") };
	}

	// cursol position
	self->cursorOrigin = infoEx.dwCursorPosition;

	return { self, std::nullopt };
}

OptionalError View::Render()
{
	// TODO move to here
	this->ShowInputBuffer();
	// TODO error
	return std::nullopt;
}


View::View(std::shared_ptr<InputBuffer> ib)
	: inputBuffer(ib)
{
}


void View::ShowInputBuffer()
{
	if (!this->enabled) {
		return;
	}
	if (!this->inputBuffer->ConsumeUpdatedFlag()) {
		return;
	}

	// windows size to get screen width
	auto [windowSize, windowErr] = ConsoleUtil::GetWindowSize();
	if (windowErr) {
		fwprintf(stderr, L"Failed to GetWindowSize\n\t%s (%d)\n\tRender@view\n", windowErr->message.c_str(), windowErr->code);
		return;
	}
	// output
	auto width = windowSize->X;
	std::wstring data = this->inputBuffer->Get();
	DWORD written = 0;
	if (!::WriteConsoleOutputCharacterW(this->stdOutHandle, data.data(), data.size(), { this->cursorOrigin.X, this->cursorOrigin.Y }, &written)) {
		auto err = ::GetLastError();
		fwprintf(stderr, L"Failed to ::WriteConsoleOutputCharacterA: %d\n", err);
		return;
	}
	// set cursor
	auto [newCursorPos, cursorErr] = ConsoleUtil::CalcCoord(this->cursorOrigin, this->inputBuffer->GetCursor());
	if (cursorErr) {
		fwprintf(stderr, L"Failed to CalcCoord\n\t%s (%d)\n", cursorErr->message.c_str(), cursorErr->code);
		return;
	}
	// debug
	//OutputDebugStringW(std::format(L"POS: ({}, {})\n", newCursorPos->X, newCursorPos->Y).c_str());
	if (!::SetConsoleCursorPosition(this->stdOutHandle, *newCursorPos)) {
		auto err = ::GetLastError();
		fwprintf(stderr, L"Failed to ::SetConsoleCursorPosition: %d\n", err);
		return;
	}

	// padding write ' ' to the end
	auto [endPos, endPosErr] = ConsoleUtil::CalcCoord(this->cursorOrigin, StringUtil::GetDisplayWidth(data));
	if (endPosErr) {
		fwprintf(stderr, L"Failed to CalcCoord\n\t%s (%d)\n", endPosErr->message.c_str(), endPosErr->code);
		return;
	}
	auto [info, infoErr] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (infoErr) {
		fwprintf(stderr, L"Failed to GetConsoleScreenBufferSize\n\t%s (%d)\n", infoErr->message.c_str(), infoErr->code);
		return;
	}
	auto [remaining, distErr] = ConsoleUtil::CalcDistance(*endPos, { info->srWindow.Right, info->srWindow.Bottom });
	if (distErr) {
		fwprintf(stderr, L"Failed to ::CalcDistance\n\t%s (%d)\n", distErr->message.c_str(), distErr->code);
		return;
	}
	written = 0;
	::FillConsoleOutputCharacterW(this->stdOutHandle, L' ', *remaining, *endPos, &written);
}


void View::ShowPrompt()
{
	if (!this->enabled) {
		return;
	}
	
	char buf[MAX_PATH];
	DWORD pathLen = ::GetCurrentDirectoryA(MAX_PATH, buf);
	if (pathLen == 0) {
		auto err = ::GetLastError();
		fprintf(stderr, "Failed to ::GetCurrentDirectory %d", err);
		return;
	}
	std::string prompt(buf, buf + pathLen);
	//prompt += "\x1b[1m:-)\x1b[0m";
	if (GetErrorLevelAsCode() == 0) {
		prompt +=  (":-)" );
	} else {
		prompt += (":-(");
	}
	DWORD written = 0;
	::WriteConsoleA(this->stdOutHandle, prompt.data(), prompt.size(), &written, nullptr);

	// update cursor pos
	auto [ info, err ] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (err) {
		fwprintf(stderr, L"Failed to ::GetConsoleScreenBufferInfo\n\t%s %d", err->message.c_str(), err->code);
		return;
	}
	this->cursorOrigin = info->dwCursorPosition;
}


void View::SetTitle()
{
	Title::Set(L"Chronicle ++++ Main ++++");
}


void View::Renew()
{
	auto [info, err] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (err) {
		fwprintf(stderr, L"Failed to ::GetConsoleScreenBufferInfo\n\t%s %d", err->message.c_str(), err->code);
		return;
	}
	// move cursor to new line
	this->cursorOrigin = { 0, SHORT(info->dwCursorPosition.Y + 1) };
	::SetConsoleCursorPosition(this->stdOutHandle, this->cursorOrigin);
}


void View::Enable(bool state)
{
	this->enabled = state;
}


View::~View()
{
}
