#pragma once
#include <functional>

class PromptGate
{
public:
	void GetReady();
	void SetOnReady(std::function<void(PromptGate*)> callback);
private:
	std::function<void(PromptGate*)> callback;
};

