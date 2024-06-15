#include "pch.h"
#include "CppUnitTest.h"

#include <optional>
#include <string>
#include "History.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace chronicletest
{
	TEST_CLASS(chronicletest)
	{
	public:
		
		TEST_METHOD(IfEmptyPrevReturnsNoValue)
		{
			History h;
			Assert::IsFalse(h.Prev().has_value());
		}
		TEST_METHOD(IfEmptyNextReturnsNoValue)
		{
			History h;
			Assert::IsFalse(h.Next().has_value());
		}
		TEST_METHOD(Prev)
		{
			History h;
			std::string val = "dummy-value";
			h.Add(val);
			Assert::AreEqual(*h.Prev(), val);
		}
		TEST_METHOD(PrevPrevNoValue)
		{
			History h;
			std::string val = "dummy-value";
			h.Add(val);
			Assert::AreEqual(val, *h.Prev());
			Assert::AreEqual(val, *h.Prev());
		}
		TEST_METHOD(PrevPrevValue)
		{
			History h;
			std::string val1 = "dummy-value1";
			std::string val2 = "dummy-value2";
			h.Add(val1);
			h.Add(val2);
			Assert::AreEqual(val2, *h.Prev());
			Assert::AreEqual(val1, *h.Prev());
		}

		TEST_METHOD(Next)
		{
			History h;
			h.Add("dummy-val");
			Assert::IsFalse(h.Next().has_value());
		}
		TEST_METHOD(PrevNextNoValue)
		{
			History h;
			std::string val = "dummy-val";
			h.Add(val);
			Assert::AreEqual(val, *h.Prev());
			Assert::AreEqual(val, *h.Next());
		}
		TEST_METHOD(PrevNextValue)
		{
			History h;
			std::string val1 = "dummy-val1";
			std::string val2 = "dummy-val2";
			h.Add(val1);
			h.Add(val2);
			Assert::AreEqual(val2, *h.Prev());
			Assert::AreEqual(val1, *h.Prev());
			Assert::AreEqual(val2, *h.Next());
		}

	};
}
