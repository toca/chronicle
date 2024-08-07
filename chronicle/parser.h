#pragma once
#include <vector>
#include <string>
#include <memory>
#include <tuple>
#include "node.h"
#include "tokenize.h"
#include "error.h"
#include "result.h" 

namespace Command
{
	class Parser
	{
	public:
		Parser(const std::vector<Token>& tokens);
		~Parser();
		std::tuple<std::shared_ptr<Node>, OptionalError> Parse();
	private:
		std::vector<Token> tokens;
		std::vector<Token>::const_iterator current;
		std::optional<Token> ConsumeText();
		std::optional<Token> ConsumeOperator();
		std::optional<Token> ConsumePipe();
		std::optional<Token> ConsumeRedirection();
		std::pair<std::wstring, std::wstring> SplitCommandAndArguments(const std::wstring& s);

		std::tuple<std::shared_ptr<Node>, OptionalError> CommandSequence();
		std::tuple<std::shared_ptr<Node>, OptionalError> CombinedCommand();
		std::tuple<std::shared_ptr<Node>, OptionalError> Command();
	};
};

