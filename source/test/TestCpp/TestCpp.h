/*
 * TestCpp.h
 *
 *  Created on: Jun 11, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

// The syntax here is somewhat close to a limited subset of Google Test. This is
// more portable since it does much less.
//
// The main usage is to define a cTestCppModule class that will represent the module that is
// being unit tested. This can hold anything that is common between tests.
//
// For each test, the TEST_F macro is used to define a test.
// Inside of each defined test, the EXPECT_EQ macro is used to perform checks.
//
// Then the runAllTests() function is called to run all defined tests in order.
//
// Usage may look like this:
//
// 		class cExampleUnitTest:public cTestCppModule
//			{};
// 		static cExampleUnitTest gExampleUnitTest;
//
//		TEST_F(gExampleUnitTest, ExampleFailTest)
//			{
//			EXPECT_EQ(5, 4);		// This will fail.
//			}
//
//		int main()
//			{ cExampleUnitTest.runAllTests(); }
//
// Output is stored in a file called TestCpp.txt that logs tests as they are run,
// and pass/fail counts at the end.



#ifndef TESTCPP_H_
#define TESTCPP_H_

#include <vector>
#include <string>
#include <sys/time.h>

class cTime
    {
    public:
	cTime()
	    { mTime.tv_sec=0; mTime.tv_usec=0; }
	cTime(timeval val):
	    mTime(val)
	    {}
	static void sleepMs(int ms);
	void getCurrentTime();
	double elapsedSecondsSinceStart(const cTime &startTime);
	operator double() const
	    { return(mTime.tv_sec + (static_cast<double>(mTime.tv_usec) / 1e6)); }

    private:
	timeval mTime;
    };

class cTestAllModules
    {
    public:
        cTestAllModules();
        ~cTestAllModules();
        static void addModule(class cTestCppModule *module);
        void runAllTests();
        static cTestAllModules *getAllModules()
            { return sTestAllModules; }
	FILE *getLogFp()
		{ return mLogFp; }

    private:
        std::vector<class cTestCppModule *> mModules;
	FILE *mLogFp;
        static cTestAllModules *sTestAllModules;
    };

// This can be derived from to hold data that is common between tests. It provides
// functions to run defined tests and logs tests as they are run.
// This accumulates counts of pass and fail and logs at the end.
class cTestCppModule
{
public:
	cTestCppModule(char const * const moduleName);
	void addFunctionTest(class cTestCppFunction *func);
	void runAllTests();
	void addExtraDiagnostics(const char *str)
	    { mExtraDiagnostics.push_back(str); }
	void addExtraDiagnostics(const char *str, double val);
        char const * const getModuleName()
            { return mModuleName; }

private:
	std::vector<class cTestCppFunction*> mTestFunctions;
	int mPassCount;
	int mFailCount;
        char const * const mModuleName;
	std::vector<std::string> mExtraDiagnostics;
};

// This will define a test function, it is instantiated with the TEST_F macro.
// When instantiated, this adds itself to the parent module to allow the parent
// module to call it.
class cTestCppFunction
{
public:
	cTestCppFunction(const char *moduleName, const char *testName, cTestCppModule &parentModule):
		mModuleName(moduleName), mTestName(testName), mParentModule(parentModule), mAnyFailure(false)
		{
		parentModule.addFunctionTest(this);
		}
	virtual ~cTestCppFunction()
		{}
	virtual void doFunctionTest(cTestCppModule &testModule) = 0;
	void testCppOutput(int lineNum, bool passed);

public:
	std::string mModuleName;
	std::string mTestName;
	cTestCppModule &mParentModule;
	bool mAnyFailure;
};


#define FUNCTION_TEST_NAME(moduleName, functionName) Test_##moduleName##_##functionName
#define FUNCTION_CLASS_NAME(moduleName, functionName) TestClass_##moduleName##_##functionName
#define INSTANTIATE_CLASS_NAME(moduleName, functionName) g_Test_##moduleName##_##functionName

// Macro to instantiate a test function. This defines a subclass of cTestCppFunction
// and instantiates it. It also defines a global test function and overrides a virtual function
// that will call it.
#define TEST_F(moduleName, functionName)			\
	struct FUNCTION_CLASS_NAME(moduleName, functionName):public cTestCppFunction		\
		{ FUNCTION_CLASS_NAME(moduleName, functionName)():\
			cTestCppFunction(#moduleName, #functionName, moduleName){} \
			virtual void doFunctionTest(cTestCppModule &testModule)	\
			{	void FUNCTION_TEST_NAME(moduleName, functionName)(cTestCppFunction&);	/* global scope function declaration */ \
			FUNCTION_TEST_NAME(moduleName, functionName)(*this); }		\
		};	\
	static FUNCTION_CLASS_NAME(moduleName, functionName) INSTANTIATE_CLASS_NAME(moduleName, functionName);	\
	void FUNCTION_TEST_NAME(moduleName, functionName)(cTestCppFunction &func)

// Macro to compare values and store in the output file.
#define EXPECT_EQ(x, y)		func.testCppOutput(__LINE__, ((x) == (y)));

#endif /* TESTCPP_H_ */
