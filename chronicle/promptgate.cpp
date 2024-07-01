#include "promptgate.h"

void PromptGate::GetReady()
{
	if (this->callback) {
		this->callback(this);
	}
}

void PromptGate::SetOnReady(std::function<void(PromptGate*)> cb)
{
	this->callback = cb;
}


