#include "searchcontroller.h"
#include "searchview.h"
#include "prompt.h"
#include "historian.h"
#include "history.h"
#include "title.h"
#include "mode.h"

SearchController::SearchController(
	std::shared_ptr<SearchView> view,
	std::shared_ptr<Prompt> prompt,
	std::shared_ptr<Historian> historian, 
	std::shared_ptr<History> history
)
	: view(view)
	, prompt(prompt)
	, historian(historian)
	, history(history)
{
}


Result<SearchController*> SearchController::Create(std::shared_ptr<History> history)
{
	auto prompt = std::make_shared<Prompt>();
	auto historian = std::make_shared<Historian>();
	auto [res, err] = SearchView::Create(prompt, historian);
	if (err) {
		return { std::nullopt, err };
	}
	SearchController* self = new SearchController(std::shared_ptr<SearchView>(*res), prompt, historian, history);
	return { self, std::nullopt };
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
						SetMode(Mode::Main);
						this->view->Reset();
						return;
					case VK_UP:
						this->historian->Next();
						break;
					case VK_DOWN:
						this->historian->Prev();
						break;
					case VK_RETURN:
					{
						auto result = this->historian->Current();

						break;
					}
					default:
						// do nothing
						break;
				}
			
				// filter histories
				this->historian->Filter(this->prompt->GetRawStr());
				
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
				//if (!this->cancelCallback) {
				//	break;
				//}
				//this->cancelCallback();
				break;
			default:
				break;
		}
	}
}


void SearchController::OnModeChanged(Mode mode)
{
	if (mode == Mode::Search) {
		this->view->SetTitle();
		this->prompt->Clear();
		this->historian->SetData(this->history->GetAll());
	}
}

