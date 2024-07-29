#pragma once
#include <functional>

enum class Mode
{
	None,
	Main,
	Search
};

void SetMode(Mode mode);
Mode GetMode();

void SetOnModeChanged(std::function<void(Mode)> callback);
