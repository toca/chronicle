#pragma once
#include <vector>
#include <string>
#include "result.h" 

namespace Command
{
	enum class NodeType
	{
		/* command and parameters */
		/* C:\Windows\System32\notepad.exe | echo | cd | etc... \*/
		Command,
		/* output.txt | C:\Users\ | NUL | etc... */
		File,
		/* | */
		Pipe,
		/* < */
		Input,
		/* > */
		Redirect,
		/* >> */
		Append,
		/* && */
		And,
		/* || */
		Or,
		/* & */
		Separator,
		/* ( commands* ) */
		Group,
		/* End of the Nodes */
		End
	};

	struct Node
	{
		NodeType type;
		std::string command;
		std::string arguments;
		std::string file;
		std::string op;
	};

	enum class TokenType
	{
		Text,
		Operator
	};

	struct Token {
		TokenType type;
		std::string data;
	};

	Result<std::vector<Node>> Parse(const std::string& input);
};

