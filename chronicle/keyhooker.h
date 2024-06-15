#pragma once
#include <Windows.h>
#include <optional>
#include <functional>
#include "error.h"

using HookListenerType = std::function<bool(KBDLLHOOKSTRUCT)>;
class KeyHooker
{
public:
	KeyHooker(HookListenerType listener);
	~KeyHooker();

	std::optional<Error> Start();
	std::optional<Error> Stop();


	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);

private:
	HWND window;
	HHOOK neighbor;
	HookListenerType listener;

};
