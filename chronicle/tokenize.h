#pragma once
#include <vector>
#include <string>

#include "result.h"
namespace Command
{
	enum class TokenKind
	{
		Text,
		Operator
	};

	struct Token {
		TokenKind kind;
		std::wstring value;
	};

	Result<std::vector<Token>> Tokenize(const std::wstring& input);
}
