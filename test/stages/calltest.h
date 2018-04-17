#ifndef SEQ_CALLTEST_H
#define SEQ_CALLTEST_H

#include "../testhelp.h"

namespace calltest {
	static bool call1 = false;
	static bool call2 = false;
	static bool call3 = false;
	static bool call4 = false;
	static bool call5 = false;
	static bool call6 = false;
	static bool call7 = false;

	static void reset()
	{
		call1 = call2 = call3 = call4 =
		  call5 = call6 = call7 = false;
	}

	struct rec_test_t {
		seq_int_t n;
		double d;
	};

	struct rec_test_nested_t {
		rec_test_t r1;
		rec_test_t r2;
	};
}

SEQ_FUNC seq_int_t callTestFunc1(seq_int_t n)
{
	calltest::call1 = true;
	return n + 1;
}

SEQ_FUNC arr_t<> callTestFunc2(seq_t s)
{
	calltest::call2 = true;
	return {0, nullptr};
}

SEQ_FUNC void callTestFunc3(seq_int_t n)
{
	calltest::call3 = true;
}

SEQ_FUNC seq_int_t callTestFunc4()
{
	calltest::call4 = true;
	return 42;
}

SEQ_FUNC void callTestFunc5()
{
	calltest::call5 = true;
}

SEQ_FUNC calltest::rec_test_t callTestFunc6()
{
	calltest::call6 = true;
	return {42, 3.14};
}

SEQ_FUNC calltest::rec_test_t callTestFunc7(calltest::rec_test_t r)
{
	calltest::call7 = true;
	return {r.n + 1, r.d + 1.0};
}

SEQ_FUNC seq_int_t multiCallTestFunc1(seq_int_t n)
{
	return n*2;
}

SEQ_FUNC double multiCallTestFunc2(seq_int_t n)
{
	return n*2.0;
}

TEST(CallTestIntInt, CallTest)
{
	calltest::reset();
	seq_int_t got = -1;

	SeqModule s;
	Func f(Int, Int, SEQ_NATIVE(callTestFunc1));
	s | count() | f() | capture(&got);
	s.source(DEFAULT_TEST_INPUT_SINGLE);
	s.execute();

	EXPECT_TRUE(calltest::call1);
	EXPECT_EQ(got, 2);
}

TEST(CallTestArrSeq, CallTest)
{
	calltest::reset();
	seq_int_t got = -1;

	SeqModule s;
	Func f(Seq, Array.of(Int), SEQ_NATIVE(callTestFunc2));
	s | f() | len() | capture(&got);
	s.source(DEFAULT_TEST_INPUT_SINGLE);
	s.execute();

	EXPECT_TRUE(calltest::call2);
	EXPECT_EQ(got, 0);
}

TEST(CallTestIntVoid, CallTest)
{
	calltest::reset();

	SeqModule s;
	Func f(Int, Void, SEQ_NATIVE(callTestFunc3));
	s | count() | f();
	s.source(DEFAULT_TEST_INPUT_SINGLE);
	s.execute();

	EXPECT_TRUE(calltest::call3);
}

TEST(CallTestVoidInt, CallTest)
{
	calltest::reset();
	seq_int_t got = -1;

	SeqModule s;
	Func f(Void, Int, SEQ_NATIVE(callTestFunc4));
	s | f() | capture(&got);
	s.source(DEFAULT_TEST_INPUT_SINGLE);
	s.execute();

	EXPECT_TRUE(calltest::call4);
	EXPECT_EQ(got, 42);
}

TEST(CallTestVoidVoid, CallTest)
{
	calltest::reset();

	SeqModule s;
	Func f(Void, Void, SEQ_NATIVE(callTestFunc5));
	s | f();
	s.source(DEFAULT_TEST_INPUT_SINGLE);
	s.execute();

	EXPECT_TRUE(calltest::call5);
}

TEST(CallTestVoidRec, CallTest)
{
	calltest::reset();
	seq_int_t intGot = -1;
	double floatGot = -1.0;

	SeqModule s;
	Func f(Void, Record.of({Int, Float}), SEQ_NATIVE(callTestFunc6));
	Var r = s | f();
	r[1] | capture(&intGot);
	r[2] | capture(&floatGot);

	s.source(DEFAULT_TEST_INPUT_SINGLE);
	s.execute();

	EXPECT_TRUE(calltest::call6);
	EXPECT_EQ(intGot, 42);
	EXPECT_EQ(floatGot, 3.14);
}

TEST(CallTestRecRec, CallTest)
{
	calltest::reset();
	seq_int_t intGot = -1;
	double floatGot = -1.0;

	SeqModule s;
	Func f1(Void, Record.of({Int, Float}), SEQ_NATIVE(callTestFunc6));
	Func f2(Record.of({Int, Float}),
	        Record.of({Int, Float}),
	        SEQ_NATIVE(callTestFunc7));

	Var r1 = s | f1();
	Var r2 = r1 | f2();
	r2[1] | capture(&intGot);
	r2[2] | capture(&floatGot);

	s.source(DEFAULT_TEST_INPUT_MULTI);
	s.execute();

	EXPECT_TRUE(calltest::call7);
	EXPECT_EQ(intGot, 42 + 1);
	EXPECT_EQ(floatGot, 3.14 + 1.0);
}

TEST(MultiCallTestStandard, MultiCallTest)
{
	seq_int_t intGot = -1;
	double floatGot = -1.0;

	SeqModule s;
	Func f1(Int, Int, SEQ_NATIVE(multiCallTestFunc1));
	Func f2(Int, Float, SEQ_NATIVE(multiCallTestFunc2));

	Var r = s | range(42,43) | (f1, f2)();
	r[1] | capture(&intGot);
	r[2] | capture(&floatGot);

	s.source(DEFAULT_TEST_INPUT_MULTI);
	s.execute();

	EXPECT_EQ(intGot, 42 * 2);
	EXPECT_EQ(floatGot, 42 * 2.0);
}

TEST(MultiCallTestNested, MultiCallTest)
{
	calltest::rec_test_nested_t recGot = {};

	SeqModule s;
	Func f1(Int, Int, SEQ_NATIVE(multiCallTestFunc1));
	Func f2(Int, Float, SEQ_NATIVE(multiCallTestFunc2));
	Func f3(Record.of({Int, Float}),
	        Record.of({Int, Float}),
	        SEQ_NATIVE(callTestFunc7));

	Var r1 = s | range(42,43) | (f1, f2)();
	Var r2 = r1 | (f3, f3)();
	r2 | capture(&recGot);

	s.source(DEFAULT_TEST_INPUT_MULTI);
	s.execute();

	EXPECT_EQ(recGot.r1.n, 42*2 + 1);
	EXPECT_EQ(recGot.r1.d, 42*2.0 + 1.0);
	EXPECT_EQ(recGot.r2.n, 42*2 + 1);
	EXPECT_EQ(recGot.r2.d, 42*2.0 + 1.0);
}

#endif /* SEQ_CALLTEST_H */
