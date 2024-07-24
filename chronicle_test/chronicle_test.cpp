#include "pch.h"
#include "CppUnitTest.h"

#include <iostream>
#include <optional>
#include <string>
#include "history.h"
#include "command.h"
#include "parser.h"
#include "tokenize.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace chronicletest
{
	using namespace Command;

	TEST_CLASS(chronicletest)
	{
	public:
		
		TEST_METHOD(IfEmptyOlderReturnsNoValue)
		{
			History h;
			Assert::IsFalse(h.Older().has_value());
		}
		TEST_METHOD(IfEmptyNewerReturnsNoValue)
		{
			History h;
			Assert::IsFalse(h.Newer().has_value());
		}
		TEST_METHOD(Older)
		{
			History h;
			std::string val = "dummy-value";
			h.Add(val);
			Assert::AreEqual(*h.Older(), val);
		}
		TEST_METHOD(OlderOlderNoValue)
		{
			History h;
			std::string val = "dummy-value";
			h.Add(val);
			Assert::AreEqual(val, *h.Older());
			Assert::IsFalse(h.Older().has_value());
		}
		TEST_METHOD(OlderOlderValue)
		{
			History h;
			std::string val1 = "dummy-value1";
			std::string val2 = "dummy-value2";
			h.Add(val1);
			h.Add(val2);
			Assert::AreEqual(val2, *h.Older());
			Assert::AreEqual(val1, *h.Older());
		}

		TEST_METHOD(Newer)
		{
			History h;
			h.Add("dummy-val");
			Assert::IsFalse(h.Newer().has_value());
		}
		TEST_METHOD(OlderNewerNoValue)
		{
			History h;
			std::string val = "dummy-val";
			h.Add(val);
			Assert::AreEqual(val, *h.Older());
			Assert::IsFalse(h.Newer().has_value());
		}
		TEST_METHOD(OlderNewerValue)
		{
			History h;
			std::string val1 = "dummy-val1";
			std::string val2 = "dummy-val2";
			h.Add(val1);
			h.Add(val2);
			Assert::AreEqual(val2, *h.Older());
			Assert::AreEqual(val1, *h.Older());
			Assert::AreEqual(val2, *h.Newer());
		}

		TEST_METHOD(Uniqueness)
		{
			History h;
			std::string val1 = "dummy-val1";
			std::string val2 = "dummy-val2";
			std::string val3 = "dummy-val1";
			h.Add(val1);
			h.Add(val2);
			h.Add(val3);

			Assert::AreEqual(val1, *h.Older());
			Assert::AreEqual(val2, *h.Older());
			Assert::IsFalse(h.Older().has_value());
		}


		TEST_METHOD(Load_items)
		{
			History h;
			std::stringstream ss("dum-val1\ndum-val2\n");
			h.Load(ss);
			Assert::AreEqual(std::string("dum-val1"), *h.Older());
			Assert::AreEqual(std::string("dum-val2"), *h.Older());
		}

		TEST_METHOD(Load_and_Add) 
		{
			History h;
			std::stringstream ss("dum-val1\ndum-val2\n");
			h.Load(ss);
			h.Add("dum-val3");
			//Assert::IsTrue(h.Older()->compare("dum-val3"));
			Assert::AreEqual(std::string("dum-val3"), *h.Older());
			Assert::AreEqual(std::string("dum-val1"), *h.Older());
			Assert::AreEqual(std::string("dum-val2"), *h.Older());
		}

		TEST_METHOD(Dump_items)
		{
			History h;
			h.Add("dum-val1");
			h.Add("dum-val2");
			h.Add("dum-val3");
			std::stringstream ss;
			h.Dump(ss);
			Assert::IsTrue(ss.str() == "dum-val3\ndum-val2\ndum-val1\n");
		}

	};

	TEST_CLASS(tokenizetest)
	{
		TEST_METHOD(SingleCommand)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir");
			Assert::IsFalse(tokenErr.has_value());
			Assert::AreEqual(size_t(1), tokens->size());
			Assert::AreEqual(std::string("dir"), tokens->at(0).value);
			Assert::IsTrue(tokens->at(0).kind == TokenKind::Text);
		}
		TEST_METHOD(ValidRedirect)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo foo 2> out.txt");
			Assert::IsFalse(tokenErr.has_value());
			Assert::AreEqual(size_t(3), tokens->size());
			Assert::AreEqual(std::string("echo foo "), tokens->at(0).value);
			Assert::IsTrue(tokens->at(0).kind == TokenKind::Text);
			Assert::AreEqual(std::string("2>"), tokens->at(1).value);
			Assert::IsTrue(tokens->at(1).kind == TokenKind::Operator);
			Assert::AreEqual(std::string("out.txt"), tokens->at(2).value);
			Assert::IsTrue(tokens->at(2).kind == TokenKind::Text);
		}
		TEST_METHOD(InvalidRedirect)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo foo2> out.txt");
			Assert::IsFalse(tokenErr.has_value());
			Assert::AreEqual(size_t(3), tokens->size());
			Assert::AreEqual(std::string("echo foo2"), tokens->at(0).value);
			Assert::IsTrue(tokens->at(0).kind == TokenKind::Text);
			Assert::AreEqual(std::string(">"), tokens->at(1).value);
			Assert::IsTrue(tokens->at(1).kind == TokenKind::Operator);
			Assert::AreEqual(std::string("out.txt"), tokens->at(2).value);
			Assert::IsTrue(tokens->at(2).kind == TokenKind::Text);
		}
		TEST_METHOD(QuotedArg)
		{
			auto [tokens, tokenErr] = Command::Tokenize("findstr \"^\"");
			Assert::IsFalse(tokenErr.has_value());
			Assert::AreEqual(size_t(2), tokens->size());
			Assert::AreEqual(std::string("findstr \"\""), tokens->at(0).value);
			Assert::IsTrue(tokens->at(0).kind == TokenKind::Text);
		}
		TEST_METHOD(Input)
		{
			auto [tokens, tokenErr] = Command::Tokenize("findstr \"key\" < test.txt");
			Assert::IsFalse(tokenErr.has_value());
			Assert::AreEqual(size_t(3), tokens->size());
			Assert::AreEqual(std::string("findstr \"key\" "), tokens->at(0).value);
			Assert::IsTrue(tokens->at(0).kind == TokenKind::Text);
			Assert::AreEqual(std::string("<"), tokens->at(1).value);
			Assert::IsTrue(tokens->at(1).kind == TokenKind::Operator);
			Assert::AreEqual(std::string("test.txt"), tokens->at(2).value);
			Assert::IsTrue(tokens->at(2).kind == TokenKind::Text);
		}
		TEST_METHOD(InputWithEscape)
		{
			auto [tokens, tokenErr] = Command::Tokenize("findstr \"^\" < input.txt");
			Assert::IsFalse(tokenErr.has_value());
			Assert::AreEqual(size_t(3), tokens->size());
			Assert::AreEqual(std::string("findstr \"\" "), tokens->at(0).value);
			Assert::IsTrue(tokens->at(0).kind == TokenKind::Text);
			Assert::AreEqual(std::string("<"), tokens->at(1).value);
			Assert::IsTrue(tokens->at(1).kind == TokenKind::Operator);
			Assert::AreEqual(std::string("input.txt"), tokens->at(2).value);
			Assert::IsTrue(tokens->at(2).kind == TokenKind::Text);
		}
		TEST_METHOD(Empty)
		{
			auto [tokens, err] = Command::Tokenize("");
			Assert::IsFalse(err.has_value());
			Assert::AreEqual(size_t(0), tokens->size());
		}

	};

	TEST_CLASS(parsertest)
	{
		const std::string SYNTAX_ERROR_MESSAGE = "The syntax of the command is incorrect.";
		TEST_METHOD(OnlySingleCommand)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [ node, err ] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(node->command, std::string("dir"));
		}
		TEST_METHOD(CommandWithArgs)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir /?");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(node->command, std::string("dir"));
			Assert::AreEqual(node->arguments, std::string("/?"));
		}
		TEST_METHOD(OneCommandOneArgWithSpace)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo    foo");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(std::string("echo"), node->command);
			Assert::AreEqual(std::string("   foo"), node->arguments);
		}
		TEST_METHOD(CommandWithQuote)
		{
			auto [tokens, tokenErr] = Command::Tokenize("\"C:\\Program Files\\Example\\command.exe\"");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(node->command, std::string("\"C:\\Program Files\\Example\\command.exe\""));
		}
		TEST_METHOD(InputRedirection)
		{
			auto [tokens, tokenErr] = Command::Tokenize("findstr \"a\" < input.txt");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(std::string("findstr"), node->command);
			Assert::AreEqual(std::string("\"a\" "),node->arguments);
			Assert::AreEqual(size_t(1), node->redirections.size());
			Assert::AreEqual(std::string("input.txt"), node->redirections.at(0).file);
			Assert::AreEqual(std::string("<"), node->redirections.at(0).op);
		}
		TEST_METHOD(InputRedirections)
		{
			auto [tokens, tokenErr] = Command::Tokenize("findstr \"a\" < input1.txt < input2.txt < input3.txt");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(std::string("findstr"), node->command);
			Assert::AreEqual(std::string("\"a\" "), node->arguments);
			Assert::AreEqual(size_t(3), node->redirections.size());
			Assert::AreEqual(std::string("input1.txt"), node->redirections.at(0).file);
			Assert::AreEqual(std::string("<"), node->redirections.at(0).op);
			Assert::AreEqual(std::string("input2.txt"), node->redirections.at(1).file);
			Assert::AreEqual(std::string("<"), node->redirections.at(1).op);
			Assert::AreEqual(std::string("input3.txt"), node->redirections.at(2).file);
			Assert::AreEqual(std::string("<"), node->redirections.at(2).op);
		}
		TEST_METHOD(OutputRedirection)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo hello> output.txt");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(std::string("echo"), node->command);
			Assert::AreEqual(std::string("hello"), node->arguments);
			Assert::AreEqual(size_t(1), node->redirections.size());
			Assert::AreEqual(std::string("output.txt"), node->redirections.at(0).file);
			Assert::AreEqual(std::string(">"), node->redirections.at(0).op);
		}
		TEST_METHOD(OutputRedirections)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo hello> output1.txt > output2.txt > output3.txt");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(std::string("echo"), node->command);
			Assert::AreEqual(std::string("hello"), node->arguments);
			Assert::AreEqual(size_t(3), node->redirections.size());
			Assert::AreEqual(std::string("output1.txt"), node->redirections.at(0).file);
			Assert::AreEqual(std::string(">"), node->redirections.at(0).op);
			Assert::AreEqual(std::string("output2.txt"), node->redirections.at(1).file);
			Assert::AreEqual(std::string(">"), node->redirections.at(1).op);
			Assert::AreEqual(std::string("output3.txt"), node->redirections.at(2).file);
			Assert::AreEqual(std::string(">"), node->redirections.at(2).op);
		}
		TEST_METHOD(AppendtRedirection)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo hello>> output.txt");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(std::string("echo"), node->command);
			Assert::AreEqual(std::string("hello"), node->arguments);
			Assert::AreEqual(size_t(1), node->redirections.size());
			Assert::AreEqual(std::string("output.txt"), node->redirections.at(0).file);
			Assert::AreEqual(std::string(">>"), node->redirections.at(0).op);
		}
		TEST_METHOD(StdErrRedirection)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo hello 2> output.txt");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(std::string("echo"), node->command);
			Assert::AreEqual(std::string("hello "), node->arguments);
			Assert::AreEqual(size_t(1), node->redirections.size());
			Assert::AreEqual(std::string("output.txt"), node->redirections.at(0).file);
			Assert::AreEqual(std::string("2>"), node->redirections.at(0).op);
		}
		TEST_METHOD(StdOutRedirection)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo hello 1> output.txt");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());
			Assert::IsTrue(node->type == NodeType::Command);
			Assert::AreEqual(std::string("echo"), node->command);
			Assert::AreEqual(std::string("hello "), node->arguments);
			Assert::AreEqual(size_t(1), node->redirections.size());
			Assert::AreEqual(std::string("output.txt"), node->redirections.at(0).file);
			Assert::AreEqual(std::string("1>"), node->redirections.at(0).op);
		}
		

		TEST_METHOD(ConbinedCommandSimple)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir | findstr \"a\"");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());

			Assert::IsTrue(node->type == NodeType::Pipe);
			Assert::IsTrue(node->left->type == NodeType::Command);
			Assert::IsTrue(node->right->type == NodeType::Command);
			Assert::AreEqual(node->left->command, std::string("dir"));
			Assert::AreEqual(node->right->command, std::string("findstr"));
		}
		TEST_METHOD(ConbinedCommandNest)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir | findstr \"a\" | more");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());

			Assert::IsTrue(node->type == NodeType::Pipe);
			Assert::IsTrue(node->left->type == NodeType::Command);
			Assert::AreEqual(std::string("dir"), node->left->command);
			Assert::IsTrue(node->right->type == NodeType::Pipe);
			Assert::IsTrue(node->right->left->type == NodeType::Command);
			Assert::AreEqual(std::string("findstr"), node->right->left->command);
			Assert::IsTrue(node->right->right->type == NodeType::Command);
			Assert::AreEqual(std::string("more"), node->right->right->command);
		}
		
		
		TEST_METHOD(CommandSequenceAnd) 
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir && cd");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());

			Assert::IsTrue(node->type == NodeType::And);
			Assert::IsTrue(node->left->type == NodeType::Command);
			Assert::AreEqual(std::string("dir"), node->left->command);
			Assert::IsTrue(node->right->type == NodeType::Command);
			Assert::AreEqual(std::string("cd"), node->right->command);
		}
		TEST_METHOD(CommandSequenceOr) 
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir || cd");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());

			Assert::IsTrue(node->type == NodeType::Or);
			Assert::IsTrue(node->left->type == NodeType::Command);
			Assert::AreEqual(std::string("dir"), node->left->command);
			Assert::IsTrue(node->right->type == NodeType::Command);
			Assert::AreEqual(std::string("cd"), node->right->command);
		}
		TEST_METHOD(CommandSequenceSeparate) 
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir & cd");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);

			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());

			Assert::IsTrue(node->type == NodeType::Separator);
			Assert::IsTrue(node->left->type == NodeType::Command);
			Assert::AreEqual(std::string("dir"), node->left->command);
			Assert::IsTrue(node->right->type == NodeType::Command);
			Assert::AreEqual(std::string("cd"), node->right->command);
		}
		TEST_METHOD(CommandSequenceErrorAnd)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir &");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);

			auto [node, err] = parser.Parse();
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(SYNTAX_ERROR_MESSAGE, err->message);
		}
		TEST_METHOD(CommandSequenceErrorOr)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir ||");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);

			auto [node, err] = parser.Parse();
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(SYNTAX_ERROR_MESSAGE, err->message);
		}
		TEST_METHOD(CommandSequenceErrorSeparater)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir &");
			Assert::IsFalse(tokenErr.has_value());
			Parser parser(*tokens);

			auto [node, err] = parser.Parse();
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(SYNTAX_ERROR_MESSAGE, err->message);
		}
		TEST_METHOD(CommandSequenceComplex)
		{
			auto [tokens, tokenErr] = Command::Tokenize("dir & cd && echo foo || path");
			Assert::IsFalse(tokenErr.has_value());

			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::IsFalse(err.has_value());

			Assert::IsTrue(node->type == NodeType::Separator);

			Assert::IsTrue(node->left->type == NodeType::Command);
			Assert::AreEqual(std::string("dir"), node->left->command);
			
			auto c = std::move(node->right);
			Assert::IsTrue(c->type == NodeType::And);

			Assert::IsTrue(c->left->type == NodeType::Command);
			Assert::AreEqual(std::string("cd"), c->left->command);

			c = std::move(c->right);
			Assert::IsTrue(c->type == NodeType::Or);

			Assert::IsTrue(c->left->type == NodeType::Command);
			Assert::AreEqual(std::string("echo"), c->left->command);

			Assert::IsTrue(c->right->type == NodeType::Command);
			Assert::AreEqual(std::string("path"), c->right->command);
		}
		
		TEST_METHOD(InvalidSeparator)
		{
			auto [tokens, tokenErr] = Command::Tokenize("&");
			Assert::IsFalse(tokenErr.has_value());

			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(std::string("& was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidAndOp)
		{
			auto [tokens, tokenErr] = Command::Tokenize("&&");
		
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(std::string("&& was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidOrOp)
		{
			auto [tokens, tokenErr] = Command::Tokenize("||");

			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(std::string("|| was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidPipeOp)
		{
			auto [tokens, tokenErr] = Command::Tokenize("|");
			
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(std::string("| was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidRedirectOp)
		{
			auto [tokens, tokenErr] = Command::Tokenize(">");
			
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(SYNTAX_ERROR_MESSAGE, err->message);
		}
		TEST_METHOD(InvalidAppendOp)
		{
			auto [tokens, tokenErr] = Command::Tokenize(">>");
			
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(SYNTAX_ERROR_MESSAGE, err->message);
		}
		TEST_METHOD(InvalidInputOp)
		{
			auto [tokens, tokenErr] = Command::Tokenize("<");
			
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(SYNTAX_ERROR_MESSAGE, err->message);
		}
		TEST_METHOD(InvalidDoubleOperator)
		{
			auto [tokens, tokenErr] = Command::Tokenize("echo foo & |");
			
			Parser parser(*tokens);
			auto [node, err] = parser.Parse();
			Assert::AreEqual(std::string("| was unexpected at this time."), err->message);
		}

		//TEST_METHOD(OnlySingleCommand)
		//{
		//	auto [ nodes, err ] = Command::Parse("dir");
		//	Assert::IsFalse(err.has_value());
		//	Assert::AreEqual(size_t(2), nodes->size());
		//	Assert::AreEqual("dir", nodes->at(0).command.c_str());
		//}
		//TEST_METHOD(OneCommandOneArg)
		//{
		//	auto [nodes, err] = Command::Parse("cd .");
		//	Assert::IsFalse(err.has_value());
		//	Assert::AreEqual(size_t(2), nodes->size());
		//	Assert::AreEqual("cd", nodes->at(0).command.c_str());
		//	Assert::AreEqual(".", nodes->at(0).arguments.c_str());
		//}
		
		//TEST_METHOD(CommandsWithPipe) 
		//{
		//	auto [nodes, err] = Command::Parse("echo foo | cat");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Pipe == nodes->at(1).type);
		//	Assert::AreEqual(std::string("cat"), nodes->at(2).command);
		//}
		//TEST_METHOD(CommandsWithOrOp)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo || echo bar");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Or == nodes->at(1).type);
		//	Assert::AreEqual(std::string("echo"), nodes->at(2).command);
		//	Assert::AreEqual(std::string("bar"), nodes->at(2).arguments);
		//}
		//TEST_METHOD(CommandsWithSeparator)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo & echo bar");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Separator == nodes->at(1).type);
		//	Assert::AreEqual(std::string("echo"), nodes->at(2).command);
		//	Assert::AreEqual(std::string("bar"), nodes->at(2).arguments);
		//}
		//TEST_METHOD(CommandsWithAndOp)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo && echo bar");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::And == nodes->at(1).type);
		//	Assert::AreEqual(std::string("echo"), nodes->at(2).command);
		//	Assert::AreEqual(std::string("bar"), nodes->at(2).arguments);
		//}
		//TEST_METHOD(CommandsWithRedirect)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo > filename");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Redirect == nodes->at(1).type);
		//	Assert::IsTrue(NodeType::File == nodes->at(2).type);
		//	Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		//}
		//TEST_METHOD(CommandsWithAppend)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo >> filename");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Append == nodes->at(1).type);
		//	Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		//}
		//TEST_METHOD(CommandsWithInput)
		//{
		//	auto [nodes, err] = Command::Parse("more < filename");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("more"), nodes->at(0).command);
		//	Assert::AreEqual(std::string(""), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Input == nodes->at(1).type);
		//	Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		//}
		//TEST_METHOD(RedirectStdOut)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo 1> filename");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Redirect == nodes->at(1).type);
		//	Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		//}
		//TEST_METHOD(AppendStdOut)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo 1>> filename");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Append == nodes->at(1).type);
		//	Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		//}
		//TEST_METHOD(RedirectStdErr)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo 2> filename");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Redirect == nodes->at(1).type);
		//	Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		//}
		//TEST_METHOD(AppendStdErr)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo 2>> filename");
		//	Assert::AreEqual(size_t(4), nodes->size());
		//	Assert::AreEqual(std::string("echo"), nodes->at(0).command);
		//	Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
		//	Assert::IsTrue(NodeType::Append == nodes->at(1).type);
		//	Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		//}
		//TEST_METHOD(RequireFile)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo >");
		//	Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
		//	Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		//}
		//TEST_METHOD(RequireFileAppend)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo >>");
		//	Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
		//	Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		//}
		//TEST_METHOD(RequireCommandPipe)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo |");
		//	Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
		//	Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		//}
		//TEST_METHOD(RequireCommandOr)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo ||");
		//	Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
		//	Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		//}
		//TEST_METHOD(RequireCommand)
		//{
		//	// "echo foo &" is valid for cmd.exe.
		//	auto [nodes, err] = Command::Parse("echo foo &");
		//	Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
		//	Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		//}
		//TEST_METHOD(RequireCommandShort)
		//{
		//	auto [nodes, err] = Command::Parse("echo foo &&");
		//	Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
		//	Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		//}

		//TEST_METHOD(Complex) 
		//{
		//	//echo Starting && (dir /b | findstr "txt" > output.txt && type output.txt) || echo No txt files found & echo Done >> log.txt

		//	auto res = Command::Parse("echo foo & type test.txt | ")
		//	Assert::IsTrue(true);
		//}
	};


	// for commandtest
	HANDLE originalStdIn, originalStdOut, originalStdErr;
	HANDLE pipeStdInRead, pipeStdInWrite;
	HANDLE pipeStdOutRead, pipeStdOutWrite;
	HANDLE pipeStdErrRead, pipeStdErrWrite;

	TEST_CLASS(commandtest)
	{
		// TODO write test 2>, 2>>, 1> 1>> 
		HANDLE OpenFileForRead(const std::string& path)
		{
			return ::CreateFileA(
				path.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			);
		}
		
		TEST_METHOD_INITIALIZE(setup)
		{
			// replace std handles to read from test code.
			originalStdIn = ::GetStdHandle(STD_INPUT_HANDLE);
			originalStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
			originalStdErr = ::GetStdHandle(STD_ERROR_HANDLE);

			::CreatePipe(&pipeStdInRead, &pipeStdInWrite, nullptr, 0);
			::CreatePipe(&pipeStdOutRead, &pipeStdOutWrite, nullptr, 0);
			::CreatePipe(&pipeStdErrRead, &pipeStdErrWrite, nullptr, 0);

			SetHandleInformation(pipeStdOutWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
			SetHandleInformation(pipeStdErrWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
			SetHandleInformation(pipeStdInRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
			DWORD handle = 0;
			auto res = GetHandleInformation(pipeStdOutWrite, &handle);

			// Redirect standard input, output, and error to the pipes
			SetStdHandle(STD_INPUT_HANDLE, pipeStdInRead);
			SetStdHandle(STD_OUTPUT_HANDLE, pipeStdOutWrite);
			SetStdHandle(STD_ERROR_HANDLE, pipeStdErrWrite);
		}
		TEST_METHOD_CLEANUP(teardown)
		{
			SetStdHandle(STD_INPUT_HANDLE, originalStdIn);
			SetStdHandle(STD_OUTPUT_HANDLE, originalStdOut);
			SetStdHandle(STD_ERROR_HANDLE, originalStdErr);

			// Close pipe handles
			CloseHandle(pipeStdInRead);
			CloseHandle(pipeStdInWrite);
			CloseHandle(pipeStdOutRead);
			CloseHandle(pipeStdOutWrite);
			CloseHandle(pipeStdErrRead);
			CloseHandle(pipeStdErrWrite);
		}
		
		TEST_METHOD(OnlySingleCommand)
		{
			auto [res, err] = Command::Execute("echo foo");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL readRes = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(readRes);
			Assert::AreEqual(0, strcmp(buf.data(), "foo\n"));
		}
		TEST_METHOD(Redirect)
		{
			auto [res, err] = Command::Execute("echo test redirect > output.txt");
			Assert::IsFalse(err.has_value());
			HANDLE h = ::CreateFileA(
				"output.txt",
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			);
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL readRes = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(readRes);
			Assert::AreEqual(0, strcmp(buf.data(), "test redirect \n"));
			::CloseHandle(h);
			system("del output.txt");
		}
		TEST_METHOD(Append)
		{
			auto [res1, err1] = Command::Execute("echo one> output.txt");
			Assert::IsFalse(err1.has_value());
			auto [res2, err2] = Command::Execute("echo two>> output.txt");
			HANDLE h = OpenFileForRead("output.txt");
			Assert::IsFalse(err2.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL readRes = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(readRes);
			Assert::AreEqual(0, strcmp(buf.data(), "one\ntwo\n"));
			::CloseHandle(h);
			system("del output.txt");
		}
		TEST_METHOD(Input)
		{
			system("echo input test> input.txt");
			auto [res, err] = Command::Execute("findstr test < input.txt");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL readRes = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(readRes);
			Assert::AreEqual(0, strcmp(buf.data(), "input test\r\n"));
			system("del input.txt");
		}
		TEST_METHOD(AndOpExecuteBoth) 
		{
			auto [res, err] = Command::Execute("echo foo && echo bar");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "foo \nbar\n"));
		}
		TEST_METHOD(AndOpExecuteFirst)
		{
			system("echo exist> output.txt");
			auto [res, err] = Command::Execute("call && del output.txt");
			Assert::IsFalse(err.has_value());
			HANDLE h = OpenFileForRead("output.txt");
			std::string buf(1024, '\0');
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(h, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));
			system("del output.txt");
		}
		TEST_METHOD(OrOpExcuteBoth) 
		{
			auto [res, err] = Command::Execute("call || echo last");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "last\n"));
		}
		TEST_METHOD(OrOpExcuteFirst)
		{
			system("echo exist> output.txt");
			auto [res, err] = Command::Execute("call || del output.txt");
			Assert::IsFalse(err.has_value());
			HANDLE h = OpenFileForRead("output.txt");
			std::string buf(1024, '\0');
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(h, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));
			system("del output.txt");
		}
		TEST_METHOD(SeparatorOpExcuteBothIfFaileFist)
		{
			auto [res, err] = Command::Execute("call & echo last");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "last\n"));
		}
		TEST_METHOD(SeparatorOpExcuteBoth)
		{
			auto [res, err] = Command::Execute("echo first&echo last");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "first\nlast\n"));
		}
		TEST_METHOD(ConsiderSkippedCommandsAsFailedWithOr)
		{
			system("echo exist> output.txt");

			auto [res, err] = Command::Execute("cd || echo foo> output.txt && del output.txt");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			HANDLE h = OpenFileForRead("output.txt");
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(h, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));

			system("del output.txt");
		}
		TEST_METHOD(ConsiderSkippedCommandsAsFailedWithAnd)
		{
			system("echo exist> output.txt");

			auto [res, err] = Command::Execute("xcopy && echo foo> output.txt && del output.txt && echo foo> output.txt");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			HANDLE h = OpenFileForRead("output.txt");
			DWORD read = 0;
			Assert::IsTrue(::ReadFile(h, buf.data(), buf.size(), &read, nullptr));
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));

			system("del output.txt");
		}


		TEST_METHOD(CurrentDirectoryShow)
		{
			auto [res, err] = Command::Execute("cd");
			Assert::IsFalse(err.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL readRes = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(readRes);
			std::string currentDir(4096, '\0');
			DWORD len = ::GetCurrentDirectoryA(currentDir.size(), currentDir.data());
			Assert::AreEqual(0, strncmp(buf.data(), currentDir.data(), len));
			//Assert::AreEqual(std::string(buf.data()), std::string(currentDir.data()));
		}
	};
}
