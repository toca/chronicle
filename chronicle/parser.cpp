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
	std::pair<std::string, std::string> SplitAtFirstSpace(const std::string& s);
	Result<NodeType> Op2Type(const std::string& op);
	bool IsRedirection(const std::string& op);

	// Parsing context 
	enum class Context
	{
		Initial,
		Command,
		Operator,
		File,
		Group
	};

	/*
		FIXME
		<command-sequence>  ::= <command> | <command> <operator> <command-sequence> | <marge> <command-sequence> |
		<command>           ::= <executable> <arguments> | <executable> <arguments> <redirections>
		<redirections>      ::= <redirector> <file> | <redirector> <file> <redirections>
		<redirector>        ::= <input-redirector> | <output-redirector>
		<input-redirector>  ::= "<"
		<output-redirector> ::= ">" | ">>"
		<marge>				::= <descriptor-from> <redirector> '&' <descriptor-to>
		<descriptor-from>   ::= "1"  |  "2"  |  ""  |
		<descriptor-to>     ::= "1"  |  "2"
		<operator>          ::= "|"  |  "||"  |  "&"  |  "&&"
		<redorector>        ::= ">"  |  ">>"
		<input>				::= "<"
		<executable>        ::= none space string or quoted string
		<arguments>         ::= any string
		<file>              ::= any string
		! Merge operations like "2>&1" are treated specially. This sequence is evaluated at runtime, not at the time of parsing.
	*/
	Result<std::vector<Node>> ParseOLD(const std::string& input)
	{
		return { std::nullopt, std::nullopt };
		//// TODO Expand Environment Variables at first.
		//auto [tokens, err] = Tokenize(input.c_str());
		//if (err) {
		//	return { std::nullopt, err };
		//}
		//std::vector<Node> result;
		//Context context = Context::Initial;
	
		//for (auto& token : *tokens) {
		//	switch (context)
		//	{
		//		case Context::Initial:
		//		case Context::Command:
		//		{
		//			if (token.kind != TokenKind::Text) {
		//				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, token.value + " was unexpected at this time.") };
		//			}
		//			auto [head, tail] = SplitAtFirstSpace(token.value);
		//			Node node(NodeType::Command);
		//			node.command = head;
		//			node.arguments = tail;
		//			result.push_back(node);
		//			context = Context::Operator;
		//			continue;
		//			break;
		//		}
		//		case Context::Operator:
		//		{
		//			if (token.kind != TokenKind::Operator) {
		//				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "LogicError Parse@parser.cpp") };
		//			}
		//			auto [type, err] = Op2Type(token.value);
		//			if (err) {
		//				return { std::nullopt, err };
		//			}
		//			Node node(*type);
		//			node.op = token.value;
		//			result.push_back(node);
		//			if (IsRedirection(token.value)) {
		//				context = Context::File;
		//			}
		//			else {
		//				context = Context::Command;
		//			}
		//			continue;
		//			break;
		//		}
		//		case Context::File:
		//		{
		//			if (token.kind != TokenKind::Text) {
		//				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, token.value + " was unexpected at this time.") };
		//			}
		//			Node node(NodeType::File);
		//			node.file = StringUtil::Trim(token.value);
		//			result.push_back(node);
		//			context = Context::Operator;
		//			continue;
		//			break;
		//		}
		//		case Context::Group:
		//		{
		//			break;
		//		}
		//		default:
		//		{
		//			return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "LogicalError Parse@parse.cpp") };
		//		}
		//	}
		//}
		//if (context == Context::Command || context == Context::File) {
		//	return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "The syntax of the command is incorrect.") };
		//}
		//result.push_back(Node(NodeType::End));
		//return { result, std::nullopt };
	}


	std::pair<std::string, std::string> SplitAtFirstSpace(const std::string& s)
	{
		size_t pos = s.find(' ');
		if (pos == std::string::npos) {
			return { s, "" };
		}
		return { s.substr(0, pos), s.substr(pos + 1) };
	}


	Result<NodeType> Op2Type(const std::string& op)
	{
		if (op == "|")    return { NodeType::Pipe, std::nullopt };
		if (op == "||")   return { NodeType::Or, std::nullopt };
		if (op == "&")    return { NodeType::Separator, std::nullopt };
		if (op == "&&")   return { NodeType::And, std::nullopt };
		//if (op == ">")    return { NodeType::Redirect, std::nullopt };
		//if (op == ">>")   return { NodeType::Append, std::nullopt };
		//if (op == "<")    return { NodeType::Input, std::nullopt };
		//if (op == "1>")   return { NodeType::Redirect, std::nullopt };
		//if (op == "1>>")  return { NodeType::Append, std::nullopt };
		//if (op == "2>")   return { NodeType::Redirect, std::nullopt };
		//if (op == "2>>")  return { NodeType::Append, std::nullopt };

		return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "Unknown Operator Op2Type@parse.cpp") };
	}


	bool IsRedirection(const std::string& op)
	{
		if (op == ">")   return true;
		if (op == ">>")  return true;
		if (op == "<")   return true;
		if (op == "1>")  return true;
		if (op == "1>>") return true;
		if (op == "2>")  return true;
		if (op == "2>>") return true;

		return false;
	}
	



	constexpr const char* SYNTAX_ERROR_MESSAGE = "The syntax of the command is incorrect.";

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
			if (this->current->value == "&" || this->current->value == "&&" || this->current->value == "||") {
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
		if (this->current->kind == TokenKind::Operator && this->current->value == "|") {
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
		if (this->current->value == "<" || this->current->value == ">" || this->current->value == ">>"
			|| this->current->value == "1>" || this->current->value == "1>>"
			|| this->current->value == "2>" || this->current->value == "2>>"
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


	std::pair<std::string, std::string> Parser::SplitCommandAndArguments(const std::string& s)
	{
		bool quote = false;
		const char* c = s.data();
		for (size_t i = 0; i < s.size(); i++) {
			if (c[i] == '"') {
				quote = !quote;
				continue;
			}
			if (!quote && c[i] == ' ') {
				return { s.substr(0, i), s.substr(i + 1) };
			}
		}
		return { s, "" };
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
	std::tuple<std::unique_ptr<Node>, OptionalError> Parser::Parse()
	{
		auto [node, err] = this->CommandSequence();
		return { std::move(node), err };
	}


	/*
	* <command-sequence> ::= <combined-command> | <combined-command> <operator> <command-sequence> 
	*/
	std::tuple<std::unique_ptr<Node>, OptionalError> Parser::CommandSequence()
	{
		// left
		auto [left, leftErr] = this->CombinedCommand();
		if (leftErr) {
			return { nullptr, leftErr };
		}

		// operator
		auto token = this->ConsumeOperator();
		if (!token) {
			return { std::move(left), std::nullopt };
		}

		// own
		auto [type, typeErr] = Op2Type(token->value);
		if (typeErr) {
			return { nullptr, typeErr };
		}
		auto node = std::make_unique<Node>(*type);

		// right
		auto [right, rightErr] = this->CommandSequence();
		if (rightErr) {
			return { nullptr, rightErr };
		}

		// expect right command-sequence
		if (!right) {
			return { nullptr, Error(ERROR_INVALID_FUNCTION, SYNTAX_ERROR_MESSAGE) };
		}

		node->left = std::move(left);
		node->right = std::move(right);
		return { std::move(node), std::nullopt };
	}


	/*
	* <combined-command>  ::= <command> | <command> "|" <combined-command>
	*/
	std::tuple<std::unique_ptr<Node>, OptionalError> Parser::CombinedCommand()
	{
		// left
		auto [left, leftErr] = this->Command();
		if (leftErr) {
			return { nullptr, leftErr };
		}

		// pipe
		auto token = this->ConsumePipe();
		if (!token) {
			return { std::move(left), std::nullopt };
		}

		// own
		auto node = std::make_unique<Node>(NodeType::Pipe);

		// right
		auto [right, rightErr] = this->CombinedCommand();
		if (rightErr) {
			return { nullptr, rightErr };
		}

		// expect right command-sequence
		if (!right) {
			return { nullptr, Error(ERROR_INVALID_FUNCTION, SYNTAX_ERROR_MESSAGE) };
		}

		node->left = std::move(left);
		node->right = std::move(right);
		return { std::move(node), std::nullopt };
	}


	/*
	* <command> ::= <executable> <arguments> | <executable> <arguments> <redirections>
	*/
	std::tuple<std::unique_ptr<Node>, OptionalError> Parser::Command()
	{
		auto token = this->ConsumeText();
		if (!token) {
			auto op = this->ConsumeOperator();
			if (op) {
				return { nullptr, Error(ERROR_INVALID_FUNCTION, op->value + " was unexpected at this time.") };
			}
			op = this->ConsumePipe();
			if (op) {
				return { nullptr, Error(ERROR_INVALID_FUNCTION, op->value + " was unexpected at this time.") };
			}
			return { nullptr, Error(ERROR_INVALID_FUNCTION, SYNTAX_ERROR_MESSAGE) };
		}
		auto node = std::make_unique<Node>(NodeType::Command);
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
		return { std::move(node), std::nullopt };
	}

}
