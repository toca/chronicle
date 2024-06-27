#include "searchview.h"
#include "searchcontroller.h"
#include "prompt.h"
#include "historian.h"

#include <vector>
#include <format>


SearchView::SearchView() 
	: screenBuffers{ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE }
	, size({0, 0})
{}


std::optional<Error> SearchView::Init(const std::vector<std::string>& histories)
{
	HANDLE oh = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (oh == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return Error(err, "Failed to ::GetStdHandle(STD_OUTPUT_HANDLE");
	}
	this->stdOutHandle = oh;

	HANDLE ih = ::GetStdHandle(STD_INPUT_HANDLE);
	if (ih == INVALID_HANDLE_VALUE) {
		auto err = ::GetLastError();
		return Error(err, "Failed to ::GetStdHandle(STD_INPUT_HANDLE");
	}
	this->stdInHandle = ih;


	// size
	CONSOLE_SCREEN_BUFFER_INFOEX infoEx{};
	infoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	::GetConsoleScreenBufferInfoEx(this->stdOutHandle, &infoEx);

	this->col = infoEx.srWindow.Right + 1;
	this->row = infoEx.srWindow.Bottom + 1;
	this->windowSize = { short(this->col), short(this->row) };

	// screen buffer
	HANDLE buffer1 = ::CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
	HANDLE buffer2 = ::CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
	this->screenBuffers[0] = buffer1;
	this->screenBuffers[1] = buffer2;

	DWORD mode = 0;
	::GetConsoleMode(buffer1, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	::SetConsoleMode(buffer1, mode);
	::SetConsoleMode(buffer2, mode);

	// init controller and models
	this->prompt = std::make_shared<Prompt>();
	this->historian = std::make_shared<Historian>(histories, this->windowSize.Y - 1);
	this->controller = std::make_unique<SearchController>(this->prompt, this->historian);
	this->controller->OnCancel([this]()
		{
			this->stop = true;
		}
	);
	this->controller->OnCompleted([this](const std::string& s)
		{
			this->stop = true;
			this->result = s;
		}
	);

	return std::nullopt;
}



Result<std::string> SearchView::Show(const std::vector<std::string>& histories)
{
	this->result = std::nullopt;

	std::string title(1024, '\0');
	::GetConsoleTitleA(title.data(), title.size());
	::SetConsoleTitleA("Chronicle ++++ Searching ++++");
	const auto error = this->Init(histories);
	if (error) {
		return { std::nullopt, error };
	}
	
	CONSOLE_SCREEN_BUFFER_INFO screenBufInfo{};
	if (!::GetConsoleScreenBufferInfo(this->stdOutHandle, &screenBufInfo)) {
		auto err = ::GetLastError();
		return { std::nullopt, Error(err, "Failed to ::GetConsoleScreenBufferInfo") };
	}

	this->size = { short(screenBufInfo.srWindow.Right - screenBufInfo.srWindow.Left + 1), short(screenBufInfo.srWindow.Bottom - screenBufInfo.srWindow.Top + 1) };


	// main loop ---- ---- ---- ----
	this->stop = false;
	while (!this->stop) {
		auto [inputs, err] = this->Read();
		if (err) {
			return { std::nullopt, err };
		}
		this->controller->Input(inputs);

		this->Render();
		Sleep(10);
	}
	// loop end ---- ---- ---- ----
	// stop
	::SetConsoleActiveScreenBuffer(this->stdOutHandle);

	// restore title
	::SetConsoleTitleA(title.c_str());
	return { this->result, std::nullopt };
}



void SearchView::Stop()
{
	this->stop = true;
}


std::tuple<std::vector<INPUT_RECORD>, std::optional<Error>> SearchView::Read()
{
	// close if size change
	//          escape
	//          mouse input? etc...
	// read input
	// managre buffer, cursor, bs, delete, etc...
	// history finder set 

	this->stdInHandle = ::GetStdHandle(STD_INPUT_HANDLE);
	DWORD count = 0;
	if(!::GetNumberOfConsoleInputEvents(this->stdInHandle, &count)) {
		return { {}, Error(::GetLastError(), "Failed to ::GetNumberOfConsoleInputEvents") };
	}
	if (!count) {
		return { {}, std::nullopt };
	}
	std::vector<INPUT_RECORD> inputs(count);
	DWORD len = DWORD(inputs.size());
	DWORD numOfEvents = 0;

	::ReadConsoleInputA(this->stdInHandle, inputs.data(), len, &numOfEvents);
	return { inputs, std::nullopt };

}



std::optional<Error> SearchView::Render()
{
	// check models updated
	if (!this->prompt->NeedUpdate() && !this->historian->NeedUpdate()) {
		return std::nullopt;
	}
	this->prompt->ResetUpdateStatus();
	this->historian->ResetUpdateStatus();

	
	// --------------------------------------
	//   5th item
	//   4th item
	//   3rd item
	//   2nd item
	// > 1st item
	// > Prompt here
	// --------------------------------------

	std::vector<std::string> lines{ uint64_t(this->size.Y), "" };
	// --PROMPT--
	lines[lines.size() - 1] = this->prompt->Get();
	size_t last = lines.size() - 1 - 1;

	size_t lineIndex = 0;
	for (int i = this->historian->Top(); i <= this->historian->Bottom(); i++) {
		auto r = this->historian->At(i);
		if (r) {
			if (r->selected) {
				// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
				lines[last - lineIndex++] += std::format("\x1b[1m\x1b[31m>\x1b[0m \x1b[1m{}\x1b[0m", r->data);
			}
			else {
				lines[last - lineIndex++] += "  " + r->data;
			}
		}

	}


	// rendering
	HANDLE back = this->screenBuffers[this->screenIndex ^ 1];
	// clear buffer as ' '
	DWORD written = 0;
	::FillConsoleOutputCharacterA(back, ' ', this->windowSize.X * this->windowSize.Y, { 0, 0 }, &written);

	::SetConsoleCursorPosition(back, { 0, 0 }); // WriteConsole starts to output from cursor pos
	SHORT y = 0;
	for (auto& line : lines) {
		DWORD written = 0;
		auto r = ::WriteConsoleA(back, line.data(), line.size(), &written, nullptr);
		::SetConsoleCursorPosition(back, { 0, ++y });
	}
	::SetConsoleCursorPosition(back, { this->prompt->GetCursor(), SHORT(this->size.Y - 1) });

	// change console buffer
	::SetConsoleActiveScreenBuffer(back);
	this->screenIndex ^= 1;

	return std::nullopt;
}


SearchView::~SearchView()
{
	::SetConsoleActiveScreenBuffer(this->stdOutHandle);
	::CloseHandle(this->screenBuffers[0]);
	::CloseHandle(this->screenBuffers[1]);
}
