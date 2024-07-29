#include "searchview.h"
#include <vector>
#include <format>

#include "searchcontroller.h"
#include "prompt.h"
#include "historian.h"
#include "title.h"



SearchView::SearchView() 
	: screenBuffers{ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE }
{
}


Result<SearchView*> SearchView::Create(std::shared_ptr<Prompt> prompt, std::shared_ptr<Historian> historian)
{
	SearchView* self = new SearchView();

	self->prompt = prompt;
	self->historian = historian;

	self->prompt->SetOnChanged([self]()
		{
			self->Render();
		}
	);
	self->historian->SetOnChanged([self]()
		{
			self->Render();
		}
	);

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

	
	// screen buffer
	HANDLE buffer1 = ::CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
	HANDLE buffer2 = ::CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
	self->screenBuffers[0] = buffer1;
	self->screenBuffers[1] = buffer2;

	DWORD mode = 0;
	::GetConsoleMode(buffer1, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(buffer1, mode);
	::SetConsoleMode(buffer2, mode);

	return { self, std::nullopt };
}


std::optional<Error> SearchView::Render()
{
	// size
	CONSOLE_SCREEN_BUFFER_INFOEX infoEx{};
	infoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	::GetConsoleScreenBufferInfoEx(this->stdOutHandle, &infoEx);

	this->col = infoEx.srWindow.Right - infoEx.srWindow.Left + 1;
	this->row = infoEx.srWindow.Bottom - infoEx.srWindow.Top + 1;
	this->windowSize = { short(this->col), short(this->row) };

	this->historian->SetMaxRowCount(this->windowSize.Y - 1);

	// --------------------------------------
	//   5th item
	//   4th item
	//   3rd item
	//   2nd item
	// > 1st item
	// > Prompt here
	// --------------------------------------

	std::vector<std::wstring> lines{ uint64_t(this->windowSize.Y), L"" };
	// --PROMPT--
	lines[lines.size() - 1] = this->prompt->Get();
	size_t last = lines.size() - 1 - 1;

	size_t lineIndex = 0;
	for (int i = this->historian->Top(); i <= this->historian->Bottom(); i++) {
		auto r = this->historian->At(i);
		if (r) {
			if (r->selected) {
				// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
				lines[last - lineIndex++] += std::format(L"\x1b[1m\x1b[31m>\x1b[0m \x1b[1m{}\x1b[0m", r->data);
			}
			else {
				lines[last - lineIndex++] += L"  " + r->data;
			}
		}

	}


	// rendering
	HANDLE back = this->screenBuffers[this->screenIndex ^ 1];
	// clear buffer as ' '
	DWORD written = 0;
	::FillConsoleOutputCharacterA(back, L' ', this->windowSize.X * this->windowSize.Y, { 0, 0 }, &written);

	::SetConsoleCursorPosition(back, { 0, 0 }); // WriteConsole starts to output from cursor pos
	SHORT y = 0;
	for (auto& line : lines) {
		DWORD written = 0;
		auto r = ::WriteConsoleA(back, line.data(), line.size() * sizeof(wchar_t), &written, nullptr);
		::SetConsoleCursorPosition(back, { 0, ++y });
	}
	::SetConsoleCursorPosition(back, { this->prompt->GetCursor(), SHORT(this->windowSize.Y - 1) });

	// change console buffer
	::SetConsoleActiveScreenBuffer(back);
	this->screenIndex ^= 1;

	return std::nullopt;
}


void SearchView::Reset()
{
	::SetConsoleActiveScreenBuffer(this->stdOutHandle);
}


void SearchView::SetTitle()
{
	Title::Set(L"Chronicle ++++ Searching ++++");
}


SearchView::~SearchView()
{
	::SetConsoleActiveScreenBuffer(this->stdOutHandle);
	::CloseHandle(this->screenBuffers[0]);
	::CloseHandle(this->screenBuffers[1]);
}
