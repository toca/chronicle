#include "searchview.h"
#include <vector>
#include <format>

#include "searchcontroller.h"
#include "inputbuffer.h"
#include "inputbufferwindow.h"
#include "historian.h"
#include "title.h"
#include "stringutil.h"
#include "consoleutil.h"



SearchView::SearchView() 
	: screenBuffers{ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE }
{
}


Result<SearchView*> SearchView::Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<Historian> historian)
{
	SearchView* self = new SearchView();

	// Setup Models
	self->inputBuffer = std::make_shared<InputBufferWindow>(inputBuffer);
	self->historian = historian;


	// stdout
	self->stdOutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (self->stdOutHandle == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, L"Failed to ::GetStdHandle(STD_OUTPUT_HANDLE)@searchview") };
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

	// Set Window Size
	//self->OnWindowSizeEvent();

	return { self, std::nullopt };
}


std::optional<Error> SearchView::Render()
{
	if (!this->enabled) {
		return std::nullopt;
	}
	bool inputChanged = this->inputBuffer->ConsumeUpdatedFlag();
	bool historyChanged = this->historian->ConsumeUpdatedFlag();
	if (!inputChanged && !historyChanged) {
		return std::nullopt;
	}

	// --------------------------------------
	//   5th item
	//   4th item
	//   3rd item
	//   2nd item
	// > 1st item
	// > Prompt here
	// --------------------------------------
	
	const size_t maxWidth = this->col - 2; // 2 = size of '  '
	std::vector<std::wstring> lines{ uint64_t(this->windowSize.Y), L"" };
	// Prompt
	static const std::wstring p = L"\x1b[96m>\x1b[0m ";
	lines[lines.size() - 1] = p + this->inputBuffer->Get();
	size_t last = lines.size() - 1 - 1;

	// Data for display
	size_t lineIndex = 0;
	for (int i = this->historian->Top(); i <= this->historian->Bottom(); i++) {
		auto r = this->historian->At(i);
		if (r) {
			if (r->selected) {
				// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
				lines[last - lineIndex++] += std::format(L"\x1b[1m\x1b[31m>\x1b[0m \x1b[1m{}\x1b[0m", StringUtil::TruncateString(r->data, maxWidth));
			}
			else {
				lines[last - lineIndex++] += L"  " + StringUtil::TruncateString(r->data, maxWidth);
			}
		}

	}


	// rendering ----
	HANDLE back = this->screenBuffers[this->screenIndex ^ 1];
	// clear buffer as ' '
	DWORD written = 0;
	::FillConsoleOutputCharacterA(back, L' ', this->windowSize.X * this->windowSize.Y, { 0, 0 }, &written);


	::SetConsoleCursorPosition(back, { 0, 0 }); // WriteConsole starts to output from cursor pos
	SHORT y = 0;
	for (auto& line : lines) {
		DWORD written = 0;
		auto r = ::WriteConsoleW(back, line.data(), line.size(), &written, nullptr);
		::SetConsoleCursorPosition(back, { 0, ++y });
	}
	static const SHORT offset = 2; // length of ' >'
	::SetConsoleCursorPosition(back, { SHORT(this->inputBuffer->GetCursor() + offset), SHORT(this->windowSize.Y - 1) });

	// change console buffer
	::SetConsoleActiveScreenBuffer(back);
	this->screenIndex ^= 1;

	return std::nullopt;
}


void SearchView::SetTitle()
{
	Title::Set(L"Chronicle ++++ Searching ++++");
}


void SearchView::Enable(bool state)
{
	this->enabled = state;
	if (this->enabled) {
		OnWindowSizeEvent();
	} else {
		::SetConsoleActiveScreenBuffer(this->stdOutHandle);
	}
}


void SearchView::OnWindowSizeEvent()
{
	auto [info, err] = ConsoleUtil::GetConsoleScreenBufferInfo(this->screenBuffers[this->screenIndex]);
	if (err) {
		fwprintf(stderr, L"Failed to GetWindowSize@searchview: %s\n", err->message.c_str());
		return;
	}
	COORD size = { SHORT(info->srWindow.Right - info->srWindow.Left + 1), SHORT(info->srWindow.Bottom - info->srWindow.Top + 1) };
	if (this->windowSize.X != size.X || this->windowSize.Y != size.Y) {
		this->row = size.Y;
		this->col = size.X;
		this->windowSize = size;

		// Bug Windows API
		// https://stackoverflow.com/questions/35901572/setconsolescreenbufferinfoex-bug
		info->srWindow.Right++;
		info->srWindow.Bottom++;
		// Update the other window size
		::SetConsoleScreenBufferInfoEx(this->screenBuffers[this->screenIndex ^ 1], &info.value());
		
		// -1 = line of input buffer
		this->historian->SetMaxRow(this->windowSize.Y - 1); 
		// -2 = "> "
		this->inputBuffer->SetWidth(col - 2);
	}
}


SearchView::~SearchView()
{
	::SetConsoleActiveScreenBuffer(this->stdOutHandle);
	::CloseHandle(this->screenBuffers[0]);
	::CloseHandle(this->screenBuffers[1]);
}
