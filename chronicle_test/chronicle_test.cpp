#include "pch.h"
#include "CppUnitTest.h"

#include <iostream>
#include <optional>
#include <string>
#include "History.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace chronicletest
{
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
}
