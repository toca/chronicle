#include "view.h"

#include <Windows.h>

#include <vector>
#include <string>
#include <sstream>
#include <cstdio>

#include "result.h"
#include "inputbuffer.h"
#include "promptgate.h"


Result<CONSOLE_SCREEN_BUFFER_INFOEX> GetConsoleScreenBufferInfo()
{
	auto stdoutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (stdoutHandle == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, "Failed to ::GetStdHandle") };
	}
	CONSOLE_SCREEN_BUFFER_INFOEX info{};
	info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	if (!::GetConsoleScreenBufferInfoEx(stdoutHandle, &info)) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, "Failed to ::GetConsoleScreenBufferInfoEx") };
	}
	return { info, std::nullopt };
}


Result<COORD> GetWindowSize()
{
	auto [info, err] = GetConsoleScreenBufferInfo();
	if (err) {
		return { std::nullopt, err };
	}
	return { COORD { SHORT(info->srWindow.Left - info->srWindow.Right + 1), SHORT(info->srWindow.Bottom - info->srWindow.Top + 1) }, std::nullopt };
}

/*
* Caluculate distans from 2 coordinate in console screen.
*/
Result<size_t> CalcDistance(const COORD& from, const COORD& to)
{
	auto [info, err] = GetConsoleScreenBufferInfo();
	if (err) {
		return { std::nullopt, err };
	}
	auto width = info->srWindow.Right - info->srWindow.Left + 1;
	auto dy = int64_t(to.Y - from.Y);
	auto dx = int64_t(to.X - from.X);
	return { dy * width + dx, std::nullopt };
}


/*
* Caluculate coordinate
*/
Result<COORD> CalcCoord(const COORD& origin, int scalar)
{
	auto [info, err] = GetConsoleScreenBufferInfo();
	if (err) {
		return { std::nullopt, err };
	}
	auto width = info->srWindow.Right - info->srWindow.Left + 1;
	int dy = scalar / width;
	int dx = scalar % width;
	return { COORD{ SHORT(origin.X + dx), SHORT(origin.Y + dy) }, std::nullopt };
}








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
	auto [ windowSize, err ] = GetWindowSize();

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
	auto [newCursorPos, cursorErr] = CalcCoord(this->cursorOrigin, this->inputBuffer->GetCursor());
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
	auto [ remaining, distErr ] = CalcDistance({ SHORT(this->cursorOrigin.X + dx), SHORT(this-> cursorOrigin.Y + dy) }, { windowSize->X, windowSize->Y });
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
	auto [ info, err ] = GetConsoleScreenBufferInfo();
	if (err) {
		fprintf(stderr, "Failed to ::GetConsoleScreenBufferInfo\n\t%s %d", err->message, err->code);
		return;
	}
	this->cursorOrigin = info->dwCursorPosition;
}


View::~View()
{
}

OptionalError View::Render()
{
	//if (PromptGate::IsReady()) {
	//	auto showErr = this->ShowPrompt();
	//	if (showErr) return showErr;
	//	auto saveErr = this->SaveCurorPosition();
	//	if (saveErr) return saveErr;
	//}

	//// no change
	//if (!this->inputBuffer->Updated()) {
	//	return std::nullopt;
	//}
	//else {
	//	this->inputBuffer->Reset();
	//}


	//::SetConsoleCursorPosition(this->stdOutHandle, this->cursorOrigin);
	//DWORD written = 0;
	//auto buffer = this->inputBuffer->Get();
	//auto r = ::WriteConsoleA(this->stdOutHandle, buffer.data(), buffer.size(), &written, nullptr);
	//::SetConsoleCursorPosition(this->stdOutHandle, { SHORT(this->cursorOrigin.X + this->inputBuffer->GetCursor()), this->cursorOrigin.Y });

	//::SetConsoleCursorPosition(back, { this->inputBuffer->GetCursor(), SHORT(this->windowSize.Y - 1) });
	//DWORD w = 0;
	return std::nullopt;
	// 全体を書く必要がるか？標準出力だけではだめ？



	//std::vector<std::string> lines{ uint64_t(this->windowSize.Y), "" };
	//// --PROMPT--
	//lines[lines.size() - 1] = this->inputBuffer->Get();
	//size_t last = lines.size() - 1 - 1;

	//size_t lineIndex = 0;
	//for (int i = this->historian->Top(); i <= this->historian->Bottom(); i++) {
	//	auto r = this->historian->At(i);
	//	if (r) {
	//		if (r->selected) {
	//			// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
	//			lines[last - lineIndex++] += std::format("\x1b[1m\x1b[31m>\x1b[0m \x1b[1m{}\x1b[0m", r->data);
	//		}
	//		else {
	//			lines[last - lineIndex++] += "  " + r->data;
	//		}
	//	}

	//}


	//// rendering
	//HANDLE back = this->screenBuffers[this->screenIndex ^ 1];
	//// clear buffer as ' '
	//DWORD written = 0;
	//::FillConsoleOutputCharacterA(back, ' ', this->windowSize.X * this->windowSize.Y, { 0, 0 }, &written);

	//::SetConsoleCursorPosition(back, { 0, 0 }); // WriteConsole starts to output from cursor pos
	//SHORT y = 0;
	//for (auto& line : lines) {
	//	DWORD written = 0;
	//	auto r = ::WriteConsoleA(back, line.data(), line.size(), &written, nullptr);
	//	::SetConsoleCursorPosition(back, { 0, ++y });
	//}
	//::SetConsoleCursorPosition(back, { this->inputBuffer->GetCursor(), SHORT(this->windowSize.Y - 1) });

	//// change console buffer
	//::SetConsoleActiveScreenBuffer(back);
	//this->screenIndex ^= 1;

	//return std::nullopt;
}
