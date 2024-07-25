#include "parser.h"
#include <vector>
#include <string>
#include "tokenize.h"
#include "node.h"
#include "result.h"
#include "error.h"
#include "stringutil.h"


namespace Command
{
	std::pair<std::wstring, std::wstring> SplitAtFirstSpace(const std::wstring& s);
	Result<NodeType> Op2Type(const std::wstring& op);
	bool IsRedirection(const std::wstring& op);


	std::pair<std::wstring, std::wstring> SplitAtFirstSpace(const std::wstring& s)
	{
		size_t pos = s.find(L' ');
		if (pos == std::wstring::npos) {
			return { s, L"" };
		}
		return { s.substr(0, pos), s.substr(pos + 1) };
	}


	Result<NodeType> Op2Type(const std::wstring& op)
	{
		if (op == L"|")    return { NodeType::Pipe, std::nullopt };
		if (op == L"||")   return { NodeType::Or, std::nullopt };
		if (op == L"&")    return { NodeType::Separator, std::nullopt };
		if (op == L"&&")   return { NodeType::And, std::nullopt };
		//if (op == ">")    return { NodeType::Redirect, std::nullopt };
		//if (op == ">>")   return { NodeType::Append, std::nullopt };
		//if (op == "<")    return { NodeType::Input, std::nullopt };
		//if (op == "1>")   return { NodeType::Redirect, std::nullopt };
		//if (op == "1>>")  return { NodeType::Append, std::nullopt };
		//if (op == "2>")   return { NodeType::Redirect, std::nullopt };
		//if (op == "2>>")  return { NodeType::Append, std::nullopt };

		return { std::nullopt, Error(ERROR_INVALID_FUNCTION, L"Unknown Operator Op2Type@parse.cpp") };
	}


	bool IsRedirection(const std::wstring& op)
	{
		if (op == L">")   return true;
		if (op == L">>")  return true;
		if (op == L"<")   return true;
		if (op == L"1>")  return true;
		if (op == L"1>>") return true;
		if (op == L"2>")  return true;
		if (op == L"2>>") return true;

		return false;
	}
	



	constexpr const wchar_t* SYNTAX_ERROR_MESSAGE = L"The syntax of the command is incorrect.";

	// class Parser ----
	Parser::Parser(const std::vector<Token>& tokens)
		: tokens(tokens)
	{
		this->current = this->tokens.begin();
	}

	Parser::~Parser()
	{
	}


	std::optional<Token> Parser::ConsumeText()
	{
		if (this->current == this->tokens.end()) {
			return std::nullopt;
		}
		if (this->current->kind == TokenKind::Text) {
			auto result = current;
			current++;
			return *result;
		}
		else {
			return std::nullopt;
		}
	}


	std::optional<Token> Parser::ConsumeOperator()
	{
		if (this->current == this->tokens.end()) {
			return std::nullopt;
		}
		if (this->current->kind == TokenKind::Operator) {
			if (this->current->value == L"&" || this->current->value == L"&&" || this->current->value == L"||") {
				auto result = current;
				current++;
				return *result;
			}
		}
		return std::nullopt;
	}


	std::optional<Token> Parser::ConsumePipe()
	{
		if (this->current == this->tokens.end()) {
			return std::nullopt;
		}
		if (this->current->kind == TokenKind::Operator && this->current->value == L"|") {
			auto result = current;
			current++;
			return *result;
		}
		else {
			return std::nullopt;
		}
	}


	std::optional<Token> Parser::ConsumeRedirection()
	{
		if (this->current == this->tokens.end()) {
			return std::nullopt;
		}
		if (this->current->kind != TokenKind::Operator) {
			return std::nullopt;
		}
		if (this->current->value == L"<" || this->current->value == L">" || this->current->value == L">>"
			|| this->current->value == L"1>" || this->current->value == L"1>>"
			|| this->current->value == L"2>" || this->current->value == L"2>>"
		) 
		{
			auto result = current;
			current++;
			return *result;
		}
		else {
			return std::nullopt;
		}
	}


	std::pair<std::wstring, std::wstring> Parser::SplitCommandAndArguments(const std::wstring& s)
	{
		bool quote = false;
		const wchar_t* c = s.data();
		for (size_t i = 0; i < s.size(); i++) {
			if (c[i] == L'"') {
				quote = !quote;
				continue;
			}
			if (!quote && c[i] == L' ') {
				return { s.substr(0, i), s.substr(i + 1) };
			}
		}
		return { s, L"" };
	}

	/*
	* <commaind-line>     ::= <command-sequence>
	* <command-sequence>  ::= <combined-command> | <combined-command> <operator> <command-sequence> 
	* <combined-command>  ::= <command> | <command> "|" <combined-command>
	* <command>           ::= <executable> <arguments> | <executable> <arguments> <redirections>
	* <redirections>      ::= <redirector> <file> | <redirector> <file> <redirections>
	* <redirector>        ::= <input-redirector> | <output-redirector>
	* <operator>          ::= "||"  |  "&"  |  "&&"
	* <input-redirector>  ::= "<"
	* <output-redirector> ::= ">" | ">>" | "1>" | "1>>" | "2>" |  "2>>"  |
	* <executable>        ::= none space string or quoted string
	* <arguments>         ::= any string
	* <file>              ::= any string
	* ! Merge operations like "2>&1" are treated specially. This sequence is evaluated at runtime, not at the time of parsing.
	*/
	std::tuple<std::shared_ptr<Node>, OptionalError> Parser::Parse()
	{
		auto [node, err] = this->CommandSequence();
		return { node, err };
	}


	/*
	* <command-sequence> ::= <combined-command> | <combined-command> <operator> <command-sequence> 
	*/
	std::tuple<std::shared_ptr<Node>, OptionalError> Parser::CommandSequence()
	{
		// left
		auto [left, leftErr] = this->CombinedCommand();
		if (leftErr) {
			return { nullptr, leftErr };
		}

		// operator
		auto token = this->ConsumeOperator();
		if (!token) {
			return { left, std::nullopt };
		}

		// own
		auto [type, typeErr] = Op2Type(token->value);
		if (typeErr) {
			return { nullptr, typeErr };
		}
		auto node = std::make_shared<Node>(*type);

		// right
		auto [right, rightErr] = this->CommandSequence();
		if (rightErr) {
			return { nullptr, rightErr };
		}

		// expect right command-sequence
		if (!right) {
			return { nullptr, Error(ERROR_INVALID_FUNCTION, SYNTAX_ERROR_MESSAGE) };
		}

		node->left = left;
		node->right = right;
		return { node, std::nullopt };
	}


	/*
	* <combined-command>  ::= <command> | <command> "|" <combined-command>
	*/
	std::tuple<std::shared_ptr<Node>, OptionalError> Parser::CombinedCommand()
	{
		// left
		auto [left, leftErr] = this->Command();
		if (leftErr) {
			return { nullptr, leftErr };
		}

		// pipe
		auto token = this->ConsumePipe();
		if (!token) {
			return { left, std::nullopt };
		}

		// own
		auto node = std::make_shared<Node>(NodeType::Pipe);

		// right
		auto [right, rightErr] = this->CombinedCommand();
		if (rightErr) {
			return { nullptr, rightErr };
		}

		// expect right command-sequence
		if (!right) {
			return { nullptr, Error(ERROR_INVALID_FUNCTION, SYNTAX_ERROR_MESSAGE) };
		}

		node->left = left;
		node->right = right;
		return { node, std::nullopt };
	}


	/*
	* <command> ::= <executable> <arguments> | <executable> <arguments> <redirections>
	*/
	std::tuple<std::shared_ptr<Node>, OptionalError> Parser::Command()
	{
		auto token = this->ConsumeText();
		if (!token) {
			auto op = this->ConsumeOperator();
			if (op) {
				return { nullptr, Error(ERROR_INVALID_FUNCTION, op->value + L" was unexpected at this time.") };
			}
			op = this->ConsumePipe();
			if (op) {
				return { nullptr, Error(ERROR_INVALID_FUNCTION, op->value + L" was unexpected at this time.") };
			}
			return { nullptr, Error(ERROR_INVALID_FUNCTION, SYNTAX_ERROR_MESSAGE) };
		}
		auto node = std::make_shared<Node>(NodeType::Command);
		auto [command, arguments] = this->SplitCommandAndArguments(token->value);
		node->command = command;
		node->arguments = arguments;

		// loop
		while (true) {
			auto op = this->ConsumeRedirection();
			if (!op) {
				break;
			}
			auto file = this->ConsumeText();
			if (!file) {
				return { nullptr, Error(ERROR_INVALID_FUNCTION, SYNTAX_ERROR_MESSAGE) };
			}
			Redirection r(StringUtil::Trim(file->value), op->value);
			node->redirections.push_back(r);
		}
		return { node, std::nullopt };
	}

}
