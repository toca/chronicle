#include "pch.h"
#include "CppUnitTest.h"

#include <iostream>
#include <optional>
#include <string>
#include "history.h"
#include "parse.h"
#include "command.h"

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
			Assert::IsTrue(NodeType::File == nodes->at(2).type);
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
		TEST_METHOD(RequireFile)
		{
			auto [nodes, err] = Command::Parse("echo foo >");
			Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
			Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		}
		TEST_METHOD(RequireFileAppend)
		{
			auto [nodes, err] = Command::Parse("echo foo >>");
			Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
			Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		}
		TEST_METHOD(RequireCommandPipe)
		{
			auto [nodes, err] = Command::Parse("echo foo |");
			Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
			Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		}
		TEST_METHOD(RequireCommandOr)
		{
			auto [nodes, err] = Command::Parse("echo foo ||");
			Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
			Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		}
		TEST_METHOD(RequireCommand)
		{
			// "echo foo &" is valid for cmd.exe.
			auto [nodes, err] = Command::Parse("echo foo &");
			Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
			Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		}
		TEST_METHOD(RequireCommandShort)
		{
			auto [nodes, err] = Command::Parse("echo foo &&");
			Assert::AreEqual(ERROR_INVALID_FUNCTION, long(err->code));
			Assert::AreEqual(std::string("The syntax of the command is incorrect."), err->message);
		}

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
		
		TEST_CLASS_INITIALIZE(setup)
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
		TEST_CLASS_CLEANUP(teardown)
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
			auto error = Command::Execute("echo foo");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "foo\n"));
		}
		TEST_METHOD(Redirect)
		{
			auto error = Command::Execute("echo test redirect > output.txt");
			Assert::IsFalse(error.has_value());
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
			BOOL res = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "test redirect \n"));
			::CloseHandle(h);
			system("del output.txt");
		}
		TEST_METHOD(Append)
		{
			auto error1 = Command::Execute("echo one> output.txt");
			Assert::IsFalse(error1.has_value());
			auto error2 = Command::Execute("echo two>> output.txt");
			HANDLE h = OpenFileForRead("output.txt");
			Assert::IsFalse(error2.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "one\ntwo\n"));
			::CloseHandle(h);
			system("del output.txt");
		}
		TEST_METHOD(Input)
		{
			system("echo input test> input.txt");
			auto error = Command::Execute("findstr ""^"" < input.txt");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "input test\r\n"));
			system("del input.txt");
		}
		TEST_METHOD(AndOpExecuteBoth) 
		{
			auto error = Command::Execute("echo foo && echo bar");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "foo \nbar\n"));
		}
		TEST_METHOD(AndOpExecuteFirst)
		{
			system("echo exist> output.txt");
			auto error = Command::Execute("call && del output.txt");
			Assert::IsFalse(error.has_value());
			HANDLE h = OpenFileForRead("output.txt");
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));
			system("del output.txt");
		}
		TEST_METHOD(OrOpExcuteBoth) 
		{
			auto error = Command::Execute("call || echo last");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "last\n"));
		}
		TEST_METHOD(OrOpExcuteFirst)
		{
			system("echo exist> output.txt");
			auto error = Command::Execute("call || del output.txt");
			Assert::IsFalse(error.has_value());
			HANDLE h = OpenFileForRead("output.txt");
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));
			system("del output.txt");
		}
		TEST_METHOD(SeparatorOpExcuteBothIfFaileFist)
		{
			auto error = Command::Execute("call & echo last");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "last\n"));
		}
		TEST_METHOD(SeparatorOpExcuteBoth)
		{
			auto error = Command::Execute("echo first&echo last");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "first\nlast\n"));
		}
		TEST_METHOD(ConsiderSkippedCommandsAsFailedWidOr)
		{
			system("echo exist> output.txt");

			auto error = Command::Execute("cd || echo foo> output.txt && del output.txt");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			HANDLE h = OpenFileForRead("output.txt");
			DWORD read = 0;
			BOOL res = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));

			system("del output.txt");
		}
		TEST_METHOD(ConsiderSkippedCommandsAsFailedWidAnd)
		{
			system("echo exist> output.txt");

			auto error = Command::Execute("call && echo foo> output.txt && del output.txt && echo foo> output.txt");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			HANDLE h = OpenFileForRead("output.txt");
			DWORD read = 0;
			BOOL res = ::ReadFile(h, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			Assert::AreEqual(0, strcmp(buf.data(), "exist\r\n"));

			system("del output.txt");
		}


		TEST_METHOD(CurrentDirectoryShow)
		{
			auto error = Command::Execute("cd");
			Assert::IsFalse(error.has_value());
			std::string buf(1024, '\0');
			DWORD read = 0;
			BOOL res = ::ReadFile(pipeStdOutRead, buf.data(), buf.size(), &read, nullptr);
			Assert::IsTrue(res);
			std::string currentDir(4096, '\0');
			DWORD len = ::GetCurrentDirectoryA(currentDir.size(), currentDir.data());
			Assert::AreEqual(0, strncmp(buf.data(), currentDir.data(), len));
		}
	};
}
