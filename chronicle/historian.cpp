#include "historian.h"
#include "regex"

Historian::Historian(const std::vector<std::string>& histories, size_t maxRowCount)
	: data(histories)
	, rowCount(maxRowCount)
	, index(-1)
	, top(-1)
	, bottom(-1)
{
	if (this->data.size()) {
		this->top = 0;
		this->bottom = std::min(this->rowCount - 1, this->data.size() - 1);
		this->index = 0;
	}
}


Historian::~Historian()
{
}


std::optional<Item> Historian::At(int index)
{
	if (this->Data().size() <= index) {
		return std::nullopt;
	}
	return Item{ index, this->Data()[index], this->index == index };
}


void Historian::Filter(const std::string& keyword)
{
	this->index = 0;
	this->candidates.clear();
	candidates.reserve(this->data.size());
	for (auto& each : this->data) {
		if (each.find(keyword) != std::string::npos) {
			this->candidates.push_back(each);
		}
	}
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
	this->updated = true;
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
	this->updated = true;
}

std::optional<std::string> Historian::Current()
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

bool Historian::NeedUpdate()
{
	return this->updated;
}


void Historian::ResetUpdateStatus()
{
	this->updated = false;
}

const std::vector<std::string>& Historian::Data()
{
	if (this->candidates.size()) {
		return this->candidates;
	}
	else {
		return this->data;
	}
}


