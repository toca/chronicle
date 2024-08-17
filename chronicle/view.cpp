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
#include "candidate.h"
#include "errorlevel.h"
#include "title.h"


Result<View*> View::Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<Candidate> candidate)
{
	View* self = new View(inputBuffer, candidate);

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
	auto showInputErr = this->ShowInputBuffer();
	if (showInputErr) {
		return showInputErr;
	}
	auto showCandidateErr = this->ShowCandidate();
	if (showCandidateErr) {
		return showCandidateErr;
	}
	return std::nullopt;
}


View::View(std::shared_ptr<InputBuffer> ib, std::shared_ptr<Candidate> c)
	: inputBuffer(ib)
	, candidate(c)
{
}


OptionalError View::ShowInputBuffer()
{
	if (!this->enabled) {
		return std::nullopt;
	}
	if (!this->inputBuffer->ConsumeUpdatedFlag()) {
		return std::nullopt;
	}

	// Screen Buffer Info
	auto [screenInfo, screenInfoErr] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (screenInfoErr) {
		return Error(*screenInfoErr, L"ShoeInputBuffer@view\n");
	}

	// New Cursor Position
	auto [newCursorPos, cursorErr] = ConsoleUtil::CalcCoord(this->cursorOrigin, this->inputBuffer->GetCursor());
	if (cursorErr) {
		return Error(*cursorErr, L"ShoeInputBuffer@view\n");
	}

	std::wstring input = this->inputBuffer->Get();
	// [0m     reset text style. 
	// [y;xH   move cursur
	std::wstring data = std::format(L"\x1b[{};{}H\x1b[0m{}\x1b[{};{}H", this->cursorOrigin.Y + 1, this->cursorOrigin.X + 1, input, newCursorPos->Y + 1, newCursorPos->X + 1);

	DWORD written = 0;
	if (!::WriteConsoleW(this->stdOutHandle, data.data(), data.size(), &written, nullptr)) {
		auto err = ::GetLastError();
		return Error(err, L"Failed to ::WriteConsoleOutputCharacter ShowInputBuffer@view");
	}
	// set cursor
	//auto [newCursorPos, cursorErr] = ConsoleUtil::CalcCoord(this->cursorOrigin, this->inputBuffer->GetCursor());
	//if (cursorErr) {
	//	return Error(*cursorErr, L"ShoeInputBuffer@view\n");
	//}
	//if (!::SetConsoleCursorPosition(this->stdOutHandle, *newCursorPos)) {
	//	auto err = ::GetLastError();
	//	return Error(err, L"Failed to ::SetConsoleCursorPosition");
	//}

	// padding write ' ' to the end
	auto [endPos, endPosErr] = ConsoleUtil::CalcCoord(this->cursorOrigin, StringUtil::GetDisplayWidth(data));
	if (endPosErr) {
		return Error(*endPosErr, L"ShoeInputBuffer@view\n");
	}
	//auto [info, infoErr] = ConsoleUtil::GetConsoleScreenBufferInfo();
	//if (infoErr) {
	//	return Error(*infoErr, L"ShoeInputBuffer@view\n");
	//}
	auto [remaining, distErr] = ConsoleUtil::CalcDistance(*endPos, { screenInfo->srWindow.Right, screenInfo->srWindow.Bottom });
	if (distErr) {
		return Error(*distErr, L"ShoeInputBuffer@view\n");
	}
	written = 0;
	if (!::FillConsoleOutputCharacterW(this->stdOutHandle, L' ', *remaining, *endPos, &written)) {
		return Error(::GetLastError(), L"Failed to ::FillConsoleOutputCharacter ShowInputBuffer@view");
	}
	else {
		return std::nullopt;
	}
	
}


OptionalError View::ShowCandidate()
{
	if (!this->candidate->ConsumeUpdateFlag()) {
		return std::nullopt;
	}
	std::wstring text{};
	auto candidateResult = this->candidate->Get();
	if (!candidateResult) {
		// No candidate
		text = L"!";
	}
	else {
		text = candidateResult->suggest;
	}

	// Padding
	auto [info, err] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (err) {
		return Error(*err, L"Failed to GetConsoleScreenBuffer ShowCandidate@view");
	}
	size_t displayWidth = StringUtil::GetDisplayWidth(text);
	auto [endPos, endPosError] = ConsoleUtil::CalcCoord(info->dwCursorPosition, displayWidth);
	if (endPosError) {
		return Error(*endPosError, L"Failed to GetDisplayWidth ShowCandidate@view");
	}
	auto [remaining, distErr] = ConsoleUtil::CalcDistance(*endPos, { info->srWindow.Right, info->srWindow.Bottom });
	if (distErr) {
		return Error(*distErr, L"Failed to CalcDistance ShowCandidate@view");
	}
	std::wstring padding(*remaining, L' ');

	// Current Cursor Position
	COORD cursorPos = info->dwCursorPosition;

	// Show Completion
	std::wstring s = std::format(L"\x1b[90m{}\x1b[0m{}\x1b[{};{}H", text, padding, cursorPos.Y + 1, cursorPos.X + 1); // 1 origin in Terminal Sequence 
	DWORD written = 0;
	if (!::WriteConsoleW(this->stdOutHandle, s.data(), s.size(), &written, nullptr)) {
		return Error(::GetLastError(), L"Failed to ::WriteConsole ShowCandidate@view");
	}

	return std::nullopt;
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
	DWORD written = 0;
	::WriteConsoleA(this->stdOutHandle, "\n", 1, &written, nullptr);
	auto [info, err] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (err) {
		fwprintf(stderr, L"Failed to ::GetConsoleScreenBufferInfo\n\t%s %d", err->message.c_str(), err->code);
		return;
	}
	// move cursor to new line
	this->cursorOrigin = { 0, SHORT(info->dwCursorPosition.Y + 1) };
	//::SetConsoleCursorPosition(this->stdOutHandle, this->cursorOrigin);
}


void View::Clear()
{
	auto [info, err] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (err) {
		fwprintf(stderr, L"Failed to Clear$view\n\t%s: %d\n", err->message.c_str(), err->code);
		return;
	}
	std::wstring newLine(info->srWindow.Bottom + 1, L'\n');
	fwprintf(stdout, L"%s", newLine.c_str());
	::SetConsoleCursorPosition(this->stdOutHandle, { 0, SHORT(info->dwCursorPosition.Y + 1) });
}


void View::Enable(bool state)
{
	this->enabled = state;
}


View::~View()
{
}
