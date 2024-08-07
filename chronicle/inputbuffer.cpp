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
		if (e.uChar.AsciiChar <= 31) {
			return std::nullopt;
		}
		// FIXME max length of input = 1024 
		if (e.uChar.UnicodeChar != L'\0' && this->buffer.size() < 1024) {
			for (int i = 0; i < e.wRepeatCount; i++) {
				auto it = this->buffer.begin();
				this->buffer.insert(it + this->cursorIndex, e.uChar.UnicodeChar);
				this->cursorIndex++;
			}
		}
		this->OnChanged();
	}
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
	// TODO move cursor to the end of string
	this->buffer = std::vector<wchar_t>(s.begin(), s.end());
	this->OnChanged();
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
	this->callbacks.push_back(cb);
}


void InputBuffer::OnChanged()
{
	for (auto& each : this->callbacks) {
		each(this);
	}
}


void InputBuffer::Left()
{
	if (0 < this->cursorIndex) {
		this->cursorIndex--;
		this->OnChanged();
	}
}

void InputBuffer::Right()
{
	if (this->cursorIndex < this->buffer.size()) {
		this->cursorIndex++;
		this->OnChanged();
	}
}

void InputBuffer::Back()
{
	if (0 < this->cursorIndex && 0 < this->buffer.size()) {
		this->buffer.erase(this->buffer.begin() + this->cursorIndex - 1);
		this->cursorIndex--;
		this->OnChanged();
	}
}

void InputBuffer::Del()
{
	if (this->cursorIndex + 1 <= this->buffer.size()) {
		this->buffer.erase(this->buffer.begin() + this->cursorIndex);
		this->OnChanged();
	}
}

void InputBuffer::Home()
{
	if (this->cursorIndex != 0) {
		this->cursorIndex = 0;
		this->OnChanged();
	}
}

void InputBuffer::End()
{
	if (this->cursorIndex != this->buffer.size()) {
		this->cursorIndex = this->buffer.size();
		this->OnChanged();
	}
}

void InputBuffer::Clear()
{
	if (0 < this->cursorIndex && 0 < this->buffer.size() && !this->buffer.empty()) {
		this->updated = true;
		this->buffer.clear();
		this->cursorIndex = 0;
		this->OnChanged();
	}
}
