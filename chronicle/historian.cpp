#include "historian.h"
#include "regex"
#include "functional"

Historian::Historian()
	: data({})
	, rowCount(-1)
	, index(-1)
	, top(-1)
	, bottom(-1)
{
}


Historian::~Historian()
{
}

void Historian::SetData(const std::vector<std::wstring>& histories)
{
	this->data = histories;
	this->index = 0;
	this->Updated();
}

void Historian::SetMaxRow(size_t maxRowCount)
{
	this->rowCount = maxRowCount;
	if (this->data.size()) {
		this->top = 0;
		this->bottom = std::min(this->rowCount - 1, this->data.size() - 1);
	}
	if (this->rowCount - 1 < this->index) {
		this->index = this->rowCount - 1;
	}
}

void Historian::SetOnChanged(std::function<void()> cb)
{
	this->callback = cb;
}


std::optional<Item> Historian::At(int index)
{
	if (this->Data().size() <= index) {
		return std::nullopt;
	}
	return Item{ index, this->Data()[index], this->index == index };
}


void Historian::Filter(const std::wstring& keyword)
{
	this->candidates.clear();
	this->candidates.reserve(this->data.size());
	for (auto& each : this->data) {
		if (each.find(keyword) != std::wstring::npos) {
			this->candidates.push_back(each);
		}
	}
	if (this->candidates.size() - 1 < this->index) {
		this->index = this->candidates.size() - 1;
	}
	this->Updated();
	//try {
	//	std::regex pattern(keyword);
	//	for (auto& each : this->data) {
	//		if (std::regex_search(each, pattern)) {
	//			this->candidates.push_back(each);
	//		}
	//	}
	//}
	//catch (std::regex_error ex) {
	//
	//}

}


void Historian::Next()
{
	if (this->index + 1 < this->Data().size()) {
		this->index++;
	}
	if (this->bottom < this->index) {
		this->bottom++;
		this->top++;
	}
	this->Updated();
}


void Historian::Prev()
{
	if (0 <= this->index - 1) {
		this->index--;
	}
	if (this->index < this->top) {
		this->top--;
		this->bottom--;
	}
	this->Updated();
}

std::optional<std::wstring> Historian::Current()
{
	if (0 <= this->index) {
		return this->data[this->index];
	}
	else {
		return std::nullopt;
	}
}


int Historian::Top()
{
	return this->top;
}


int Historian::Bottom()
{
	return this->bottom;
}


const std::vector<std::wstring>& Historian::Data()
{
	if (this->candidates.size()) {
		return this->candidates;
	}
	else {
		return this->data;
	}
}

void Historian::Updated()
{
	if (this->callback) {
		this->callback();
	}
}


