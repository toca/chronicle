#include "parse.h"
#include <vector>
#include <string>
#include "result.h"

namespace Command
{
	Result<std::vector<Token>> Tokenize(const std::string& input);
	bool IsOperator(const char c1, const char c2);
	std::pair<std::string, std::string> SplitAtFirstSpace(const std::string& s);
	Result<NodeType> Op2Type(const std::string& op);
	bool IsRedirection(const std::string& op);
	Result<std::string> ReadOperator(const char* s);
	Result<std::string> ReadText(const char* s);

	// Parsing context 
	enum class Context
	{
		Command,
		Operator,
		File,
		Group
	};

	/*
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
	Result<std::vector<Node>> Parse(const std::string& input)
	{
		auto [tokens, err] = Tokenize(input.c_str());
		if (err) {
			return { std::nullopt, err };
		}
		std::vector<Node> result;
		Context context = Context::Command;

		for (auto& token : *tokens) {
			switch (context)
			{
				case Context::Command:
				{
					if (token.type != TokenType::Text) {
						return { std::nullopt, Error(ERROR_INVALID_FUNCTION, token.data + " was unexpected at this time.") };
					}
					auto [head, tail] = SplitAtFirstSpace(token.data);
					result.push_back({ NodeType::Command, head, tail, "", "" });
					context = Context::Operator;
					continue;
					break;
				}
				case Context::Operator:
				{
					if (token.type != TokenType::Operator) {
						return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "LogicError Parse@parser.cpp") };
					}
					auto [type, err] = Op2Type(token.data);
					if (err) {
						return { std::nullopt, err };
					}
					result.push_back({ *type, "", "", "", token.data });
					if (IsRedirection(token.data)) {
						context = Context::File;
					}
					else {
						context = Context::Command;
					}
					continue;
					break;
				}
				case Context::File:
				{
					if (token.type != TokenType::Text) {
						return { std::nullopt, Error(ERROR_INVALID_FUNCTION, token.data + " was unexpected at this time.") };
					}
					result.push_back({ NodeType::Command, "", "", token.data, "" });
					context = Context::Operator;
					continue;
					break;
				}
				case Context::Group:
				{
					break;
				}
				default:
				{
					return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "LogicalError Parse@parse.cpp") };
				}
			}
		}
		result.push_back({ NodeType::End, "", "", "", "" });
		return { result, std::nullopt };
	}



	Result<std::vector<Token>> Tokenize(const std::string& input)
	{
		std::vector<Token> result;
		std::string data;
		const char* s = input.c_str();
		while (*s) {
			if (*s == ' ') { // ignore leading space
				s++;
				continue;
			}
			if (IsOperator(*s, *(s + 1))) {
				auto [op, err] = ReadOperator(s);
				if (err) return { std::nullopt, err };
				result.push_back({ TokenType::Operator,  *op });
				s += op->size();
			}
			else {
				auto [text, err] = ReadText(s);
				if (err) return { std::nullopt, err };
				result.push_back({ TokenType::Text, *text });
				s += text->size();
			}
		}
		return { result, std::nullopt };
	}


	bool IsOperator(const char c1, const char c2)
	{
		if (c1 == '1' || c1 == '2') {
			if (c2 == '>') {
				return true;
			}
			else {
				return false;
			}
		}

		if (c1 == '>' || c1 == '<' || c1 == '|' || c1 == '&') {
			return true;
		}
		
		return false;
	}

	/*
	* <output-redirect>		::= <descriptor-from><redirector> | <descriptor-from><redirector>'&'<descriptor-to>
	* <descriptor-from>		::= "1" | "2" | ""
	* <descriptor-to>		::= "1" | "2"
	* <output-redirector>	::= ">" | ">>"
	*/
	Result<std::string> ReadOperator(const char* s)
	{
		std::string result;
		// Expect "1>" or "2>"
		if (*s == '1' || *s == '2') {
			result += *s;
			s++;
			if (*s != '>') {
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, "LogicalError ReadOperator@parse.cpp") };
			}
		}
		// redirect
		if (*s == '>') {
			result += *s;
			s++;
			if (*s == '>') {
				result += *s;
				s++;
			}
			if (*s == '&')
			{
				result += *s;
				s++;
				if (*s == '1' || *s == '2') {
					result += *s;
				}
			}
			return { result, std::nullopt };
		}
		// input
		else if (*s == '<') {
			return { "<", std::nullopt };
		}
		else if (*s == '|') {
			if (*(s + 1) == '|') {
				return { "||", std::nullopt };
			}
			else {
				return { "|", std::nullopt };
			}
		}
		else if (*s == '&') {
			if (*(s + 1) == '&') {
				return { "&&", std::nullopt };
			}
			else {
				return { "&", std::nullopt };
			}
		}
		return { std::nullopt, Error(ERROR_BAD_FORMAT, "Failed to ReadOperator: Unknow Operator") };
	}


	Result<std::string> ReadText(const char* s)
	{
		std::string data;
		while (*s) {
			if (*s == '^') {
				data += *s;
			}
			else if (IsOperator(*s, *(s + 1))) {
				return { data, std::nullopt };
			}
			else {
				data += *s;
			}
			s++;
		}
		return { data, std::nullopt };
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
		if (op == ">")    return { NodeType::Redirect, std::nullopt };
		if (op == ">>")   return { NodeType::Append, std::nullopt };
		if (op == "<")    return { NodeType::Input, std::nullopt };
		if (op == "1>")   return { NodeType::Redirect, std::nullopt };
		if (op == "1>>")  return { NodeType::Append, std::nullopt };
		if (op == "2>")   return { NodeType::Redirect, std::nullopt };
		if (op == "2>>")  return { NodeType::Append, std::nullopt };

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
	
}
