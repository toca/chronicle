#pragma once
#include <string>
namespace Completion
{
struct Item 
{
	std::wstring text;
	std::wstring suggest;
};
}