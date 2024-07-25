#include "tokenize.h"
namespace Command
{
	bool IsOperator(const wchar_t c1);
	bool IsSpecialOperator(const wchar_t c1, const wchar_t c2);
	Result<std::wstring> ReadOperator(const wchar_t* s);
	Result<std::wstring> ReadText(const wchar_t* s);

	Result<std::vector<Token>> Tokenize(const std::wstring& input)
	{
		std::vector<Token> result;
		std::string data;
		const wchar_t* s = input.c_str();
		while (*s) {
			if (*s == L' ') { // ignore leading space
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


	bool IsOperator(const wchar_t c1)
	{
		if (c1 == L'>' || c1 == L'<' || c1 == L'|' || c1 == L'&' || c1 == L'(' || c1 == L')') {
			return true;
		}

		return false;
	}

	bool IsSpecialOperator(const wchar_t c1, const wchar_t c2)
	{
		if (c1 == L'1' || c1 == L'2') {
			if (c2 == L'>') {
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
	Result<std::wstring> ReadOperator(const wchar_t* s)
	{
		std::wstring result;
		// Expect "1>" or "2>"
		if (*s == L'1' || *s == L'2') {
			result += *s;
			s++;
			if (*s != L'>') {
				return { std::nullopt, Error(ERROR_INVALID_FUNCTION, L"LogicalError ReadOperator@parse.cpp") };
			}
		}
		// redirect
		if (*s == L'>') {
			result += *s;
			s++;
			if (*s == L'>') {
				result += *s;
				s++;
			}
			if (*s == L'&')
			{
				result += *s;
				s++;
				if (*s == L'1' || *s == L'2') {
					result += *s;
				}
			}
			return { result, std::nullopt };
		}
		// input
		else if (*s == L'<') {
			return { L"<", std::nullopt };
		}
		else if (*s == L'|') {
			if (*(s + 1) == L'|') {
				return { L"||", std::nullopt };
			}
			else {
				return { L"|", std::nullopt };
			}
		}
		else if (*s == L'&') {
			if (*(s + 1) == L'&') {
				return { L"&&", std::nullopt };
			}
			else {
				return { L"&", std::nullopt };
			}
		}
		else if (*s == L'(') {
			return { L"(", std::nullopt };
		}
		else if (*s == L')') {
			return { L")", std::nullopt };
		}
		return { std::nullopt, Error(ERROR_BAD_FORMAT, L"Failed to ReadOperator: Unknow Operator") };
	}


	Result<std::wstring> ReadText(const wchar_t* s)
	{
		std::wstring data;
		bool quote = false;
		while (*s) {
			if (*s == L'^') {
				s++;
				if (*s == L'"') {
					quote = !quote;
				}
				if (*s) {
					data += *s;
				}
			}
			else if (!quote && IsOperator(*s)) {
				return { data, std::nullopt };
			}
			else {
				if (*s == L'"') {
					quote = !quote;
				}
				else if (!quote && *s == L' ') { // expect " 2>"
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