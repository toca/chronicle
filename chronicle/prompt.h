#pragma once
#include <Windows.h>
#include <string>
#include <vector>
class Prompt
{
public:
	Prompt();
	~Prompt();
	std::wstring Get();
	std::wstring GetRawStr();
	SHORT GetCursor();
	void InputKey(const KEY_EVENT_RECORD& keyEvent);
	bool NeedUpdate();
	void ResetUpdateStatus();
private:
	std::vector<wchar_t> buffer;
	//std::vector<char>::iterator sentinel;
	//std::list<char> buffer;
	//std::list<char>::iterator sentinel;
	//std::list<char>::iterator current;
	SHORT cursorIndex = 0;
	bool updated = true;

	void Left();
	void Right();
};

