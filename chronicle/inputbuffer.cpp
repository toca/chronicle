#include "inputbuffer.h"

InputBuffer::InputBuffer()
{
}

InputBuffer::~InputBuffer()
{
}

OptionalError InputBuffer::InputKey(const KEY_EVENT_RECORD& e)
{
	if (!e.bKeyDown) {
		return std::nullopt;
	}
	this->updated = false;
	switch (e.wVirtualKeyCode)
	{
	case VK_BACK:
		this->Back();
		break;
	case VK_DELETE:
		this->Del();
		break;
	case VK_HOME:
		this->Home();
		break;
	case VK_END:
		this->End();
		break;
	case VK_TAB:
		break;
	//case VK_RETURN:
	//	e.uChar.AsciiChar;
	//	break;
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
		this->updated = true;
		// max length of input = 32
		if (e.uChar.AsciiChar != L'\0' && this->buffer.size() < 1024) {
			for (int i = 0; i < e.wRepeatCount; i++) {
				auto it = this->buffer.begin();
				this->buffer.insert(it + this->cursorIndex, e.uChar.AsciiChar);
				this->cursorIndex++;
			}
		}
	}
	this->OnChanged();
	this->updated = false;
	return std::nullopt;
}

std::string InputBuffer::Get()
{
	return std::string(this->buffer.begin(), this->buffer.end());
}

SHORT InputBuffer::GetCursor()
{
	return this->cursorIndex;
}

void InputBuffer::ClearInput()
{
	this->updated = true;
	this->cursorIndex = 0;
	this->buffer.clear();
}

void InputBuffer::SetOnChange(std::function<void(InputBuffer*)> cb)
{
	this->callback = cb;
}

void InputBuffer::OnChanged()
{
	if (this->callback && this->updated) {
		this->callback(this);
	}
}

void InputBuffer::Left()
{
	if (0 < this->cursorIndex) {
		this->updated = true;
		this->cursorIndex--;
		this->updated = true;
	}
}

void InputBuffer::Right()
{
	if (this->cursorIndex < this->buffer.size()) {
		this->updated = true;
		this->cursorIndex++;
		this->updated = true;
	}
}

void InputBuffer::Back()
{
	if (0 < this->cursorIndex && 0 < this->buffer.size()) {
		this->updated = true;
		this->buffer.erase(this->buffer.begin() + this->cursorIndex - 1);
		this->cursorIndex--;
		this->updated = true;
	}
}

void InputBuffer::Del()
{
	if (this->cursorIndex + 1 <= this->buffer.size()) {
		this->updated = true;
		this->buffer.erase(this->buffer.begin() + this->cursorIndex);
		this->updated = true;
	}
}

void InputBuffer::Home()
{
	this->cursorIndex = 0;
	this->updated = true;
}

void InputBuffer::End()
{
	this->cursorIndex = this->buffer.size();
	this->updated = true;
}
