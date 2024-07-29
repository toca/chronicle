#include "mode.h"
#include <functional>

Mode currentMode = Mode::None;
std::function<void(Mode)> onChangedCallback;

void SetMode(Mode mode)
{
	if (onChangedCallback) {
		if (mode != currentMode) {
			onChangedCallback(mode);
		}
	}
	currentMode = mode;
}

Mode GetMode()
{
	return currentMode;
}


void SetOnModeChanged(std::function<void(Mode)> callback)
{
	onChangedCallback = callback;
}

