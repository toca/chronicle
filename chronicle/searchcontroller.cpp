#include "searchcontroller.h"
#include "prompt.h"
#include "historian.h"

SearchController::SearchController(std::shared_ptr<Prompt> p, std::shared_ptr<Historian> h)
	: prompt(p)
	, historian(h)
{
}

SearchController::~SearchController()
{
}

void SearchController::Input(const std::vector<INPUT_RECORD>& inputs)
{
	for (auto& each : inputs) {
		switch (each.EventType) {
			case KEY_EVENT:
				this->prompt->InputKey(each.Event.KeyEvent);

				if (!each.Event.KeyEvent.bKeyDown) {
					continue;
				}

				switch (each.Event.KeyEvent.wVirtualKeyCode) {
					case VK_ESCAPE:
						if (this->cancelCallback) this->cancelCallback();
						break;
					case VK_UP:
						this->historian->Next();
						break;
					case VK_DOWN:
						this->historian->Prev();
						break;
					case VK_RETURN:
					{
						auto result = this->historian->Current();
						if (result && this->completedCallback) this->completedCallback(*result);
						break;
					}
					default:
						// do nothing
						break;
				}
			
				// filter histories
				if (this->prompt->NeedUpdate()) {
					this->historian->Filter(this->prompt->GetRawStr());
				}
				break;
			case WINDOW_BUFFER_SIZE_EVENT:
				if (this->windowSize.X == 0 && this->windowSize.Y == 0) {
					this->windowSize = each.Event.WindowBufferSizeEvent.dwSize;
					break;
				}
				if (each.Event.WindowBufferSizeEvent.dwSize.X == this->windowSize.X
					&& each.Event.WindowBufferSizeEvent.dwSize.Y == this->windowSize.Y)
				{
					break;
				}
				if (!this->cancelCallback) {
					break;
				}
				this->cancelCallback();
				break;
			default:
				break;
		}
	}
}

void SearchController::OnCancel(std::function<void()> callback)
{
	this->cancelCallback = callback;
}

void SearchController::OnCompleted(std::function<void(const std::wstring&)> callback)
{
	this->completedCallback = callback;
}
