#include "tokenize.h"
namespace Command
{
	bool IsOperator(const char c1);
	bool IsSpecialOperator(const char c1, const char c2);
	Result<std::string> ReadOperator(const char* s);
	Result<std::string> ReadText(const char* s);

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
			if (IsOperator(*s) || IsSpecialOperator(*s, *(s + 1))) {
				auto [op, err] = ReadOperator(s);
				if (err) return { std::nullopt, err };
				result.push_back({ TokenKind::Operator,  *op });
				s += op->size();
			}
			else {
				auto [text, err] = ReadText(s);
				if (err) return { std::nullopt, err };
				result.push_back({ TokenKind::Text, *text });
				s += text->size();
			}
		}
		return { result, std::nullopt };
	}


	bool IsOperator(const char c1)
	{
		if (c1 == '>' || c1 == '<' || c1 == '|' || c1 == '&' || c1 == '(' || c1 == ')') {
			return true;
		}

		return false;
	}

	bool IsSpecialOperator(const char c1, const char c2)
	{
		if (c1 == '1' || c1 == '2') {
			if (c2 == '>') {
				return true;
			}
		}
		return false;
	}
	/*
	* <output-redirect>		::= <descriptor-from><redirector> | <descriptor-from><redirector>'&'<descriptor-to>
	* <descriptor-from>		::= "1" | "2" | ""
	* <descriptor-to>		::= "1" | "2"
	* <output-redirector>	::= ">" | ">>"
	* Wow! Try `echo Hello2>&1World`  It's valid.
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
		else if (*s == '(') {
			return { "(", std::nullopt };
		}
		else if (*s == ')') {
			return { ")", std::nullopt };
		}
		return { std::nullopt, Error(ERROR_BAD_FORMAT, "Failed to ReadOperator: Unknow Operator") };
	}


	Result<std::string> ReadText(const char* s)
	{
		std::string data;
		bool quote = false;
		while (*s) {
			if (*s == '^') {
				s++;
				if (*s) {
					data += *s;
				}
			}
			else if (!quote && IsOperator(*s)) {
				return { data, std::nullopt };
			}
			else {
				if (*s == '"') {
					quote = !quote;
				}
				else if (!quote && *s == ' ') { // expect " 2>"
					if (*(s + 1) && *(s + 2)) {
						if (IsSpecialOperator(*(s + 1), *(s + 2))) {
							data += *s;
							return { data, std::nullopt };
						}
					}
				}
				data += *s;
			}
			s++;
		}
		return { data, std::nullopt };
	}
}