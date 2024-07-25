#pragma once
#include <vector>
#include <string>
#include <memory>

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
		//Input,
		/* > */
		//Redirect,
		/* >> */
		//Append,
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

	struct Redirection 
	{
		Redirection(const std::wstring file, const std::wstring op)
			: file(file)
			, op(op)
		{}
		std::wstring file;
		std::wstring op;
	};

	struct Node
	{
	public:
		Node(NodeType type)
			: type(type)
		{}

		NodeType type;
		std::shared_ptr<Node> left;
		std::shared_ptr<Node> right;
		std::wstring command = L"";
		std::wstring arguments = L"";
		std::wstring file = L"";
		std::wstring op = L"";
		std::vector<Redirection> redirections = {};
	};
}
