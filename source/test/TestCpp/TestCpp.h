/*
 * TestCpp.h
 *
 *  Created on: Jun 11, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

// The syntax here is somewhat close to a limited subset of Google Test. This is
// more portable since it does much less.
//
// The main usage is to define a TestCppModule class that will represent the module that is
// being unit tested. This can hold anything that is common between tests.
//
// For each test, the TEST_F macro is used to define a test.
// Inside of each defined test, the EXPECT_EQ macro is used to perform checks.
//
// Then the runAllTests() function is called to run all defined tests in order.
//
// Usage may look like this:
//
// 		class ExampleUnitTest:public TestCppModule
//			{};
// 		static ExampleUnitTest gExampleUnitTest;
//
//		TEST_F(gExampleUnitTest, ExampleFailTest)
//			{
//			EXPECT_EQ(5, 4);		// This will fail.
//			}
//
//		int main()
//			{ ExampleUnitTest.runAllTests(); }
//
// Output is stored in a file called TestCpp.txt that logs tests as they are run,
// and pass/fail counts at the end.



#ifndef TESTCPP_H_
#define TESTCPP_H_

#include <vector>
#include <string>
#include <sys/time.h>

class TestTime
    {
    public:
        TestTime()
	    { mTime.tv_sec=0; mTime.tv_usec=0; }
	TestTime(timeval val):
	    mTime(val)
	    {}
	static void sleepMs(int ms);
	void getCurrentTime();
	double elapsedSecondsSinceStart(const TestTime &startTime);
	operator double() const
	    { return(mTime.tv_sec + (static_cast<double>(mTime.tv_usec) / 1e6)); }

    private:
	timeval mTime;
    };

class TestAllModules
    {
    public:
        TestAllModules();
        ~TestAllModules();
        static void addModule(class TestCppModule *module);
        void runAllTests();
        static TestAllModules *getAllModules()
            { return sTestAllModules; }
	FILE *getLogFp()
		{ return mLogFp; }

    private:
        std::vector<class TestCppModule *> mModules;
	FILE *mLogFp;
        static TestAllModules *sTestAllModules;
    };

// This can be derived from to hold data that is common between tests. It provides
// functions to run defined tests and logs tests as they are run.
// This accumulates counts of pass and fail and logs at the end.
class TestCppModule
{
public:
	TestCppModule(char const * const moduleName);
	void addFunctionTest(class TestCppFunction *func);
    // Returns true for passed all tests.
    bool runAllTests();
	void addExtraDiagnostics(const char *str)
	    { mExtraDiagnostics.push_back(str); }
	void addExtraDiagnostics(const char *str, double val);
        char const * const getModuleName()
            { return mModuleName; }

private:
	std::vector<class TestCppFunction*> mTestFunctions;
	int mPassCount;
	int mFailCount;
        char const * const mModuleName;
	std::vector<std::string> mExtraDiagnostics;
};

// This will define a test function, it is instantiated with the TEST_F macro.
// When instantiated, this adds itself to the parent module to allow the parent
// module to call it.
class TestCppFunction
{
public:
	TestCppFunction(const char *moduleName, const char *testName, TestCppModule &parentModule):
		mModuleName(moduleName), mTestName(testName), mParentModule(parentModule), mAnyFailure(false)
		{
		parentModule.addFunctionTest(this);
		}
	virtual ~TestCppFunction()
		{}
	virtual void doFunctionTest(TestCppModule &testModule) = 0;
	void testCppOutput(int lineNum, bool passed);

public:
	std::string mModuleName;
	std::string mTestName;
	TestCppModule &mParentModule;
	bool mAnyFailure;
};


#define FUNCTION_TEST_NAME(moduleName, functionName) Test_##moduleName##_##functionName
#define FUNCTION_CLASS_NAME(moduleName, functionName) TestClass_##moduleName##_##functionName
#define INSTANTIATE_CLASS_NAME(moduleName, functionName) g_Test_##moduleName##_##functionName

// Macro to instantiate a test function. This defines a subclass of TestCppFunction
// and instantiates it. It also defines a global test function and overrides a virtual function
// that will call it.
#define TEST_F(moduleName, functionName)			\
	struct FUNCTION_CLASS_NAME(moduleName, functionName):public TestCppFunction		\
		{ FUNCTION_CLASS_NAME(moduleName, functionName)():\
			TestCppFunction(#moduleName, #functionName, moduleName){} \
			virtual void doFunctionTest(TestCppModule &testModule)	\
			{	void FUNCTION_TEST_NAME(moduleName, functionName)(TestCppFunction&);	/* global scope function declaration */ \
			FUNCTION_TEST_NAME(moduleName, functionName)(*this); }		\
		};	\
	static FUNCTION_CLASS_NAME(moduleName, functionName) INSTANTIATE_CLASS_NAME(moduleName, functionName);	\
	void FUNCTION_TEST_NAME(moduleName, functionName)(TestCppFunction &func)

// Macro to compare values and store in the output file.
#define EXPECT_EQ(x, y)		func.testCppOutput(__LINE__, ((x) == (y)));

#endif /* TESTCPP_H_ */
