#include "inputbuffer.h"
#include "stringutil.h"

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
	case VK_ESCAPE:
		this->Clear();
		break;
	case VK_HOME:
		this->Home();
		break;
	case VK_END:
		this->End();
		break;
	case VK_TAB:
		break;
	case VK_LEFT:
		this->Left();
		break;
	case VK_RIGHT:
		this->Right();
		break;
	case VK_RETURN:
	case VK_UP:
	case VK_DOWN:
		// do nothing
		break;
	default:
		this->updated = true;
		// FIXME max length of input = 1024 
		if (e.uChar.UnicodeChar != L'\0' && this->buffer.size() < 1024) {
			for (int i = 0; i < e.wRepeatCount; i++) {
				auto it = this->buffer.begin();
				this->buffer.insert(it + this->cursorIndex, e.uChar.UnicodeChar);
				this->cursorIndex++;
			}
		}
	}
	this->OnChanged();
	this->updated = false;
	return std::nullopt;
}


std::wstring InputBuffer::Get()
{
	return std::wstring(this->buffer.begin(), this->buffer.end());
}


std::wstring InputBuffer::GetCommand()
{
	std::wstring s(this->buffer.begin(), this->buffer.end());
	auto pos = s.find(L'\r');
	if (pos == std::string::npos) {
		return s;
	}
	else {
		return s.substr(0, pos);
	}
}


void InputBuffer::Set(const std::wstring& s)
{
	this->buffer = std::vector<wchar_t>(s.begin(), s.end());
	this->updated = true;
	this->OnChanged();
	this->updated = false;
}


SHORT InputBuffer::GetCursor()
{
	std::wstring s(this->buffer.begin(), this->buffer.begin() + this->cursorIndex);
	return StringUtil::GetDisplayWidth(s);
}


void InputBuffer::ClearInput()
{
	this->Clear();
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
		this->cursorIndex--;
		this->updated = true;
	}
}

void InputBuffer::Right()
{
	if (this->cursorIndex < this->buffer.size()) {
		this->cursorIndex++;
		this->updated = true;
	}
}

void InputBuffer::Back()
{
	if (0 < this->cursorIndex && 0 < this->buffer.size()) {
		this->buffer.erase(this->buffer.begin() + this->cursorIndex - 1);
		this->cursorIndex--;
		this->updated = true;
	}
}

void InputBuffer::Del()
{
	if (this->cursorIndex + 1 <= this->buffer.size()) {
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

void InputBuffer::Clear()
{
	if (0 < this->cursorIndex && 0 < this->buffer.size()) {
		this->updated = true;
		this->buffer.clear();
		this->cursorIndex = 0;
	}
}
