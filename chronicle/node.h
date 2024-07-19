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
		Redirection(const std::string file, const std::string op)
			: file(file)
			, op(op)
		{}
		std::string file;
		std::string op;
	};

	struct Node
	{
	public:
		Node(NodeType type)
			: type(type)
		{}

		NodeType type;
		std::unique_ptr<Node> left;
		std::unique_ptr<Node> right;
		std::string command = "";
		std::string arguments = "";
		std::string file = "";
		std::string op = "";
		std::vector<Redirection> redirections = {};
	};
}
