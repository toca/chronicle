#include "pch.h"
#include "CppUnitTest.h"

#include <iostream>
#include <optional>
#include <string>
#include "history.h"
#include "parse.h"

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

	TEST_CLASS(parsertest)
	{
		TEST_METHOD(OnlySingleCommand)
		{
			auto [ nodes, err ] = Command::Parse("dir");
			Assert::IsFalse(err.has_value());
			Assert::AreEqual(size_t(2), nodes->size());
			Assert::AreEqual("dir", nodes->at(0).command.c_str());
		}
		TEST_METHOD(OneCommandOneArg)
		{
			auto [nodes, err] = Command::Parse("cd .");
			Assert::IsFalse(err.has_value());
			Assert::AreEqual(size_t(2), nodes->size());
			Assert::AreEqual("cd", nodes->at(0).command.c_str());
			Assert::AreEqual(".", nodes->at(0).arguments.c_str());
		}
		TEST_METHOD(OneCommandOneArgWithSpace)
		{
			auto [nodes, err] = Command::Parse("echo    foo");
			Assert::IsFalse(err.has_value());
			Assert::AreEqual(size_t(2), nodes->size());
			Assert::AreEqual("echo", nodes->at(0).command.c_str());
			Assert::AreEqual("   foo", nodes->at(0).arguments.c_str());
		}
		TEST_METHOD(InvalidSeparatorO)
		{
			auto [nodes, err] = Command::Parse("&");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string("& was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidAndOp)
		{
			auto [nodes, err] = Command::Parse("&&");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string("&& was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidOrOp)
		{
			auto [nodes, err] = Command::Parse("||");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string("|| was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidPipeOp)
		{
			auto [nodes, err] = Command::Parse("|");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string("| was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidRedirectOp)
		{
			auto [nodes, err] = Command::Parse(">");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string("> was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidAppendOp)
		{
			auto [nodes, err] = Command::Parse(">>");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string(">> was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidInputOp)
		{
			auto [nodes, err] = Command::Parse("<");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string("< was unexpected at this time."), err->message);
		}
		TEST_METHOD(InvalidDoubleOperator)
		{
			auto [nodes, err] = Command::Parse("echo foo & |");
			Assert::IsTrue(err.has_value());
			Assert::AreEqual(std::string("| was unexpected at this time."), err->message);
		}
		TEST_METHOD(Empty)
		{
			auto [nodes, err] = Command::Parse("");
			Assert::IsFalse(err.has_value());
			Assert::AreEqual(size_t(1), nodes->size());
		}
		TEST_METHOD(CommandsWithPipe) 
		{
			auto [nodes, err] = Command::Parse("echo foo | cat");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Pipe == nodes->at(1).type);
			Assert::AreEqual(std::string("cat"), nodes->at(2).command);
		}
		TEST_METHOD(CommandsWithOrOp)
		{
			auto [nodes, err] = Command::Parse("echo foo || echo bar");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Or == nodes->at(1).type);
			Assert::AreEqual(std::string("echo"), nodes->at(2).command);
			Assert::AreEqual(std::string("bar"), nodes->at(2).arguments);
		}
		TEST_METHOD(CommandsWithSeparator)
		{
			auto [nodes, err] = Command::Parse("echo foo & echo bar");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Separator == nodes->at(1).type);
			Assert::AreEqual(std::string("echo"), nodes->at(2).command);
			Assert::AreEqual(std::string("bar"), nodes->at(2).arguments);
		}
		TEST_METHOD(CommandsWithAndOp)
		{
			auto [nodes, err] = Command::Parse("echo foo && echo bar");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::And == nodes->at(1).type);
			Assert::AreEqual(std::string("echo"), nodes->at(2).command);
			Assert::AreEqual(std::string("bar"), nodes->at(2).arguments);
		}
		TEST_METHOD(CommandsWithRedirect)
		{
			auto [nodes, err] = Command::Parse("echo foo > filename");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Redirect == nodes->at(1).type);
			Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		}
		TEST_METHOD(CommandsWithAppend)
		{
			auto [nodes, err] = Command::Parse("echo foo >> filename");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Append == nodes->at(1).type);
			Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		}
		TEST_METHOD(CommandsWithInput)
		{
			auto [nodes, err] = Command::Parse("more < filename");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("more"), nodes->at(0).command);
			Assert::AreEqual(std::string(""), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Input == nodes->at(1).type);
			Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		}
		TEST_METHOD(RedirectStdOut)
		{
			auto [nodes, err] = Command::Parse("echo foo 1> filename");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Redirect == nodes->at(1).type);
			Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		}
		TEST_METHOD(AppendStdOut)
		{
			auto [nodes, err] = Command::Parse("echo foo 1>> filename");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Append == nodes->at(1).type);
			Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		}
		TEST_METHOD(RedirectStdErr)
		{
			auto [nodes, err] = Command::Parse("echo foo 2> filename");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Redirect == nodes->at(1).type);
			Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		}
		TEST_METHOD(AppendStdErr)
		{
			auto [nodes, err] = Command::Parse("echo foo 2>> filename");
			Assert::AreEqual(size_t(4), nodes->size());
			Assert::AreEqual(std::string("echo"), nodes->at(0).command);
			Assert::AreEqual(std::string("foo "), nodes->at(0).arguments);
			Assert::IsTrue(NodeType::Append == nodes->at(1).type);
			Assert::AreEqual(std::string("filename"), nodes->at(2).file);
		}
		

		//TEST_METHOD(Complex) 
		//{
		//	//echo Starting && (dir /b | findstr "txt" > output.txt && type output.txt) || echo No txt files found & echo Done >> log.txt

		//	auto res = Command::Parse("echo foo & type test.txt | ")
		//	Assert::IsTrue(true);
		//}
	};
}
