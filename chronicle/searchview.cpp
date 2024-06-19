#include "searchview.h"

#include <vector>

SearchView::SearchView() {}

Result<SearchView*> SearchView::Create()
{
	auto self = new SearchView();

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


	// size
	CONSOLE_SCREEN_BUFFER_INFOEX infoEx{};
	infoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	::GetConsoleScreenBufferInfoEx(self->stdOutHandle, &infoEx);

	self->col = infoEx.srWindow.Right + 1;
	self->row = infoEx.srWindow.Bottom + 1;
	self->windowSize = { short(self->col), short(self->row) };

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

std::optional<Error> SearchView::Show()
{
	// read current directory to show prompt
	CONSOLE_SCREEN_BUFFER_INFO screenBufInfo{};
	if (!::GetConsoleScreenBufferInfo(this->stdOutHandle, &screenBufInfo)) {
		auto err = ::GetLastError();
		return Error(err, "Failed to ::GetConsoleScreenBufferInfo");
	}
	auto bufferSize = (screenBufInfo.dwSize.X * screenBufInfo.dwSize.Y) - (screenBufInfo.dwCursorPosition.X * screenBufInfo.dwCursorPosition.Y);
	std::string buf(bufferSize, '\0');
	DWORD len = 0;
	::ReadConsoleOutputCharacterA(this->stdOutHandle, buf.data(), buf.size(), { 0, screenBufInfo.dwCursorPosition.Y }, &len);
	auto promptPos = buf.find('>');
	if (promptPos == std::string::npos) {
		return Error(2, "LogicalError");
	}
	buf.erase(promptPos + 1);
	this->prompt = buf;

	// main loop
	this->stop = false;
	while (!this->stop) {
		auto [inputs, err] = this->Read();
		if (err) {
			return err;
		}
		else {
			this->ProcessInputs(inputs);
		}


		this->Render();
		Sleep(100);
	}
	// stop
	::SetConsoleActiveScreenBuffer(this->stdOutHandle);
	return std::nullopt;
}



void SearchView::Stop()
{
	this->stop = true;
}


std::tuple<std::vector<INPUT_RECORD>, std::optional<Error>> SearchView::Read()
{
	// close if size change
	// escape
	// mouse input? etc...
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
	std::vector<INPUT_RECORD> inputs(4);
	DWORD len = DWORD(inputs.size());
	DWORD numOfEvents = 0;

	::ReadConsoleInputA(this->stdInHandle, inputs.data(), len, &numOfEvents);
	return { inputs, std::nullopt };

}



std::optional<Error> SearchView::Render()
{
	HANDLE back = this->screenBuffers[this->screenIndex ^ 1];

	CONSOLE_SCREEN_BUFFER_INFO screenBufInfo{};
	if (!::GetConsoleScreenBufferInfo(this->stdOutHandle, &screenBufInfo)) {
		auto err = ::GetLastError();
		return Error(err, "Failed to ::GetConsoleScreenBufferInfo");
	}

	COORD screenSize = { short(screenBufInfo.srWindow.Right - screenBufInfo.srWindow.Left + 1), short(screenBufInfo.srWindow.Bottom - screenBufInfo.srWindow.Top + 1) };

	std::vector<std::string> lines{ (uint64_t)screenSize.Y, "" };
	lines[0] = this->prompt;
	lines[1] = "Hello";
	lines[2] = "World";
	lines[lines.size() - 1] = this->inputting;

	//std::vector<std::vector<CHAR_INFO>> buffer(screenSize.Y, std::vector<CHAR_INFO>(screenSize.X));
	//for (int y = 0; y < screenSize.Y; y++) {
	//	for (int x = 0; x < lines[y].size() || x < screenBufInfo.dwSize.X; x++) {
	//		if (x < lines[y].size()) {
	//			buffer[y][x].Char.AsciiChar = lines[y][x];
	//		}
	//		else {
	//			buffer[y][x].Char.AsciiChar = ' ';
	//		}
	//		buffer[y][x].Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE;
	//	}
	//}

	//SMALL_RECT writeArea = { 0, 0, screenSize.X - 1, screenSize.Y - 1 };
	//auto r = ::WriteConsoleOutputA(back, buffer[0].data(), screenSize, { 0, 0 }, &writeArea);

	std::vector<CHAR_INFO> buffer( uint64_t(screenSize.Y) * (uint64_t)screenSize.X, CHAR_INFO{} );
	for (int y = 0; y < screenSize.Y; y++) {
		for (int x = 0; x < lines[y].size() || x < screenSize.X; x++) {
			if (x < lines[y].size()) {
				buffer[((int64_t)y*screenSize.X) + x].Char.AsciiChar = lines[y][x];
			}
			else {
				buffer[((int64_t)y*screenSize.X) + x].Char.AsciiChar = ' ';
			}
			buffer[((int64_t)y * screenSize.X) + x].Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
		}
	}

	SMALL_RECT writeArea = { 0, 0, screenSize.X - 1, screenSize.Y - 1 };
	auto r = ::WriteConsoleOutputA(back, buffer.data(), screenSize, { 0, 0 }, &writeArea);



	

	// reset cursor position
	//::SetConsoleCursorPosition(back, { 0, 0 });
	//::WriteConsoleA(back, content.data(), content.size(), &written, nullptr);
	//::SetConsoleCursorPosition(back, this->cursorPos);
	::SetConsoleCursorPosition(back, { 0, screenSize.Y - (SHORT)1 });
	::SetConsoleActiveScreenBuffer(back);
	this->screenIndex ^= 1;
	return std::nullopt;
}


void SearchView::ProcessInputs(const std::vector<INPUT_RECORD> inputs)
{
	for (auto& each : inputs) {
		if (each.EventType == KEY_EVENT) {
			if (each.Event.KeyEvent.bKeyDown) {
				this->inputting += each.Event.KeyEvent.uChar.AsciiChar;
			}
		}
	}
}


SearchView::~SearchView()
{
	::SetConsoleActiveScreenBuffer(this->stdOutHandle);
	::CloseHandle(this->screenBuffers[0]);
	::CloseHandle(this->screenBuffers[1]);
}
