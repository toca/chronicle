#include "view.h"

#include <Windows.h>

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>

#include "consoleutil.h"
#include "result.h"
#include "inputbuffer.h"
#include "promptgate.h"


Result<View*> View::Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<PromptGate> promptGate)
{
	View* self = new View(inputBuffer, promptGate);

	// std handles
	HANDLE oh = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (oh == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, "Failed to ::GetStdHandle(STD_OUTPUT_HANDLE") };
	}
	self->stdOutHandle = oh;

	HANDLE ih = ::GetStdHandle(STD_INPUT_HANDLE);
	if (ih == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, "Failed to ::GetStdHandle(STD_INPUT_HANDLE") };
	}
	self->stdInHandle = ih;


	// console screen buffer info
	CONSOLE_SCREEN_BUFFER_INFOEX infoEx{};
	infoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	if (!::GetConsoleScreenBufferInfoEx(self->stdOutHandle, &infoEx)) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, "Failed to ::GetConsoleScreenBufferInfoEx") };
	}

	// cursol position
	self->cursorOrigin = infoEx.dwCursorPosition;

	// subscription
	inputBuffer->SetOnChange([self](InputBuffer*)
		{
			self->ShowInputBuffer();
		}
	);
	promptGate->SetOnReady([self](PromptGate*)
		{
			self->ShowPrompt();
		}
	);

	return { self, std::nullopt };
}


View::View(std::shared_ptr<InputBuffer> ib, std::shared_ptr<PromptGate> pg)
	: inputBuffer(ib)
	, promptGate(pg)
{
}


void View::ShowInputBuffer()
{
	SHORT dx = 0;
	SHORT dy = 0;

	// windows size to get width
	auto [ windowSize, windowErr ] = ConsoleUtil::GetWindowSize();
	if (windowErr) {
		fprintf(stderr, "Failed to GetWindowSize\n\t%s (%d)\n", windowErr->message.c_str(), windowErr->code);
		return;
	}


	// output
	auto width = windowSize->X;
	std::string data = this->inputBuffer->Get();
	bool newLine = data.find('\r') != std::string::npos;
	// proc each width
	for (size_t i = 0; i < data.size(); i += width) {
		std::string line = data.substr(i, width) + '\r';
		DWORD written = 0;
		if (!::WriteConsoleOutputCharacterA(this->stdOutHandle, line.data(), line.size(), { this->cursorOrigin.X, SHORT(this->cursorOrigin.Y + dy)}, &written)) {
			auto err = ::GetLastError();
			fprintf(stderr, "Failed to ::WriteConsoleOutputCharacterA: %d\n", err);
			return;
		}
		dx += line.size();
		dy++;
	}
		
	
	if (newLine) {
		if (!::SetConsoleCursorPosition(this->stdOutHandle, { 0, SHORT(this->cursorOrigin.Y + dy) })) {
			auto err = ::GetLastError();
			fprintf(stderr, "Failed to ::SetConsoleCursorPosition: %d\n", err);
		}
		else {
			this->cursorOrigin = { 0, SHORT(this->cursorOrigin.Y + dy) };
		}
		return;
	}

	// cursor
	auto [newCursorPos, cursorErr] = ConsoleUtil::CalcCoord(this->cursorOrigin, this->inputBuffer->GetCursor());
	if (cursorErr) {
		fprintf(stderr, "Failed to CalcCoord\n\t%s (%d)\n", cursorErr->message.c_str(), cursorErr->code);
		return;
	}
	if (!::SetConsoleCursorPosition(this->stdOutHandle, *newCursorPos)){
		auto err = ::GetLastError();
		fprintf(stderr, "Failed to ::SetConsoleCursorPosition: %d\n", err);
		return;
	}
	
	// padding write ' ' to the end
	auto [info, infoErr] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (infoErr) {
		fprintf(stderr, "Failed to GetConsoleScreenBufferSize\n\t%s (%d)\n", infoErr->message.c_str(), infoErr->code);
		return;
	}
	auto [ remaining, distErr ] = ConsoleUtil::CalcDistance({ SHORT(this->cursorOrigin.X + dx - 1), SHORT(this-> cursorOrigin.Y + dy - 1) }, { info->srWindow.Right, info->srWindow.Bottom });
	if (distErr) {
		fprintf(stderr, "Failed to ::CalcDistance\n\t%s (%d)\n", distErr->message.c_str(), distErr->code);
		return;
	}
	std::string padding(*remaining, ' ');
	DWORD written = 0;
	::WriteConsoleOutputCharacterA(this->stdOutHandle, padding.data(), padding.size(), { SHORT(this->cursorOrigin.X + dx), SHORT(this->cursorOrigin.Y + dy) }, &written);
}


void View::ShowPrompt()
{
	// TODO get real prompt
	char buf[MAX_PATH + 1]; // +1 for '>'
	DWORD pathLen = ::GetCurrentDirectoryA(MAX_PATH, buf);
	if (pathLen == 0) {
		auto err = ::GetLastError();
		fprintf(stderr, "Failed to ::GetCurrentDirectory %d", err);
		return;
	}
	buf[pathLen] = '>';
	std::string prompt(buf, buf + pathLen + 1);
	DWORD written = 0;
	::WriteConsoleA(this->stdOutHandle, prompt.data(), prompt.size(), &written, nullptr);

	// update cursor pos
	auto [ info, err ] = ConsoleUtil::GetConsoleScreenBufferInfo();
	if (err) {
		fprintf(stderr, "Failed to ::GetConsoleScreenBufferInfo\n\t%s %d", err->message.c_str(), err->code);
		return;
	}
	this->cursorOrigin = info->dwCursorPosition;
}


View::~View()
{
}
