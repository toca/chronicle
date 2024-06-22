#include "prompt.h"

#include <format>

Prompt::Prompt()
{
	// TODO
	// check window buffer width and to be able to show correct range
}

Prompt::~Prompt()
{
}

std::string Prompt::Get()
{
	static const std::string p = "\x1b[96m>\x1b[0m ";
	std::string s(this->buffer.begin() , this->buffer.end());
	return p + s;
}

std::string Prompt::GetRawStr()
{
	return std::string(this->buffer.begin(), this->buffer.end());
}

SHORT Prompt::GetCursor()
{
	return this->cursorIndex + 2; // 2 is length of '> '
}

void Prompt::InputKey(const KEY_EVENT_RECORD& e)
{
	//::OutputDebugStringW(std::format(L"down: {}\nrepat: {}\nvk: {:x}\nsc: {:x}\nUC: '{}'\nkeySt: {}\n", 
	//	e.bKeyDown, 
	//	e.wRepeatCount, 
	//	e.wVirtualKeyCode, 
	//	e.wVirtualScanCode, 
	//	//e.uChar.AsciiChar, 
	//	e.uChar.UnicodeChar,
	//	e.dwControlKeyState
	//).c_str());
	
	if (!e.bKeyDown) {
		return;
	}

	switch (e.wVirtualKeyCode)
	{
	case VK_BACK:
		if (0 < this->cursorIndex && 0 < this->buffer.size()) {
			this->buffer.erase(this->buffer.begin() + this->cursorIndex - 1);
			this->cursorIndex--;
			this->updated = true;
		}
		break;
	case VK_DELETE:
		if (this->cursorIndex + 1 <= this->buffer.size()) {
			this->buffer.erase(this->buffer.begin() + this->cursorIndex);
			this->updated = true;
		}
		break;
	case VK_HOME:
		this->cursorIndex = 0;
		this->updated = true;
		break;
	case VK_END:
		this->cursorIndex = this->buffer.size();
		this->updated = true;
		break;
	case VK_TAB:
		break;
	case VK_RETURN:
		break;
	//case VK_SPACE:
		//break;
	//case VK_UP:
		//break;
	//case VK_DOWN:
		//break;
	case VK_LEFT:
			this->Left();
		break;
	case VK_RIGHT:
			this->Right();
		break;
	//case VK_ESCAPE:
		//break;
	//case VK_TAB:
		//break;
	default:
		// max length of input = 32
		if (e.uChar.AsciiChar != L'\0' && this->buffer.size() < 32) {
			this->updated = true;
			for (int i = 0; i < e.wRepeatCount; i++) {
				auto it = this->buffer.begin();
				this->buffer.insert(it + this->cursorIndex, e.uChar.AsciiChar);
				this->cursorIndex++;
			}
		}
	}
}

bool Prompt::NeedUpdate()
{
	return this->updated;
}

void Prompt::ResetUpdateStatus()
{
	this->updated = false;
}

void Prompt::Left()
{
	if (0 < this->cursorIndex) {
		this->cursorIndex--;
		this->updated = true;
	}
}

void Prompt::Right()
{
	if (this->cursorIndex < this->buffer.size()) {
		this->cursorIndex++;
		this->updated = true;
	}
}
