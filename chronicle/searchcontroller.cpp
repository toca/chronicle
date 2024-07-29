#include "searchcontroller.h"
#include "searchview.h"
#include "inputbuffer.h"
#include "historian.h"
#include "history.h"
#include "title.h"
#include "mode.h"

SearchController::SearchController(
	std::shared_ptr<SearchView> view,
	std::shared_ptr<InputBuffer> inputBuffer,
	std::shared_ptr<Historian> historian, 
	std::shared_ptr<History> history
)
	: view(view)
	, inputBuffer(inputBuffer)
	, historian(historian)
	, history(history)
{
}


Result<SearchController*> SearchController::Create(std::shared_ptr<InputBuffer> inputBuffer, std::shared_ptr<History> history)
{
	auto historian = std::make_shared<Historian>();
	auto [res, err] = SearchView::Create(inputBuffer, historian);
	if (err) {
		return { std::nullopt, err };
	}
	SearchController* self = new SearchController(std::shared_ptr<SearchView>(*res), inputBuffer, historian, history);
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
				this->inputBuffer->InputKey(each.Event.KeyEvent);

				if (!each.Event.KeyEvent.bKeyDown) {
					continue;
				}

				switch (each.Event.KeyEvent.wVirtualKeyCode) {
					case VK_ESCAPE:
						SetMode(Mode::Main);
						this->view->Enable(false);
						this->inputBuffer->Set(this->inputting);
						return;
					case VK_UP:
						this->historian->Next();
						break;
					case VK_DOWN:
						this->historian->Prev();
						break;
					case VK_RETURN:
					{
						SetMode(Mode::Main);
						auto result = this->historian->Current();
						if (result) {
							this->inputBuffer->Set(*result);
						}
						else {
							this->inputBuffer->Set(this->inputting);
						}
						break;
					}
					default:
						// do nothing
						break;
				}
			
				// filter histories
				this->historian->Filter(this->inputBuffer->Get());
				
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
		this->view->Enable(true);
		this->inputting = this->inputBuffer->Get();
		this->historian->SetData(this->history->GetAll());
	}
	else {
		this->view->Enable(false);
	}
}

