/*
 * TestCpp.cpp
 *
 *  Created on: Jun 11, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "TestCpp.h"
#include <stdio.h>
#include <time.h>

#ifdef _X86_
#include <windows.h>		// For Sleep
#else
#include <unistd.h>		// For usleep
#endif


void TestTime::sleepMs(int ms)
{
#ifndef _X86_
	usleep(ms*1000);
#else
	Sleep(ms);
#endif
}

void TestTime::getCurrentTime()
    {
    gettimeofday(&mTime, NULL);
    }

double TestTime::elapsedSecondsSinceStart(const TestTime &startTime)
    {
    return(TestTime(mTime) - startTime);
    }

TestAllModules *TestAllModules::sTestAllModules;

TestAllModules::TestAllModules()
    {
    mLogFp = fopen("TestCpp.txt", "w");
    }

TestAllModules::~TestAllModules()
    {
    fclose(mLogFp);
    }

void TestAllModules::addModule(class TestCppModule *module)
    {
    if(!TestAllModules::sTestAllModules)
        TestAllModules::sTestAllModules = new TestAllModules();
    TestAllModules::sTestAllModules->mModules.push_back(module);
    }

void TestAllModules::runAllTests()
    {
    int passCount = 0;
    int failCount = 0;
    for(auto const &mod : mModules)
        {
        fprintf(mLogFp, "Case: %s\n", mod->getModuleName());
        if(mod->runAllTests())
            {
            passCount++;
            }
        else
            {
            failCount++;
            }
        }

    FILE *stdOutput = stdout;
    if(failCount > 0)
        {
        stdOutput = stderr;
        }
    fprintf(stdOutput, "\nModule Summary:  %d Passed, %d Failed\n\n",
        passCount, failCount);
    printf("See log for details\n");
    }

///////////////

TestCppModule::TestCppModule(char const * const moduleName):
	mPassCount(0), mFailCount(0), mModuleName(moduleName)
	{
        TestAllModules::addModule(this);
	}

void TestCppModule::addFunctionTest(class TestCppFunction *func)
	{ mTestFunctions.push_back(func); }

bool TestCppModule::runAllTests()
    {
    FILE *logFp = TestAllModules::getAllModules()->getLogFp();
    for(size_t i=0; i<mTestFunctions.size(); i++)
        {
        TestCppFunction &testFunc = *mTestFunctions[i];
        fprintf(logFp, " Test %d:%s %s\n",
            i+1, testFunc.mModuleName.c_str(), testFunc.mTestName.c_str());
        fflush(logFp);
        testFunc.doFunctionTest(*this);
        if(testFunc.mAnyFailure)
            {
            mFailCount++;
            }
        else
            {
            mPassCount++;
            }
        }
    if(mExtraDiagnostics.size() > 0)
        {
        fprintf(logFp, "\nExtra Diagnostics\n");
        for(size_t i=0; i<mExtraDiagnostics.size(); i++)
            {
            fprintf(logFp, "  %s\n", mExtraDiagnostics[i].c_str());
            }
        }
    fprintf(logFp, "\nCase Summary:  %d Passed, %d Failed\n\n",
        mPassCount, mFailCount);
    fflush(logFp);
    return(mFailCount == 0);
    }

void TestCppModule::addExtraDiagnostics(const char *str, double val)
    {
    char buf[100];
    sprintf(buf, "%s: %f", str, val);
    addExtraDiagnostics(buf);
    }

//////

void TestCppFunction::testCppOutput(int lineNum, bool passed)
    {
    const char *successStr;
    if(passed)
        successStr = "Pass";
    else
        {
        successStr = "Fail";
        mAnyFailure = true;
        }
    FILE *logFp = TestAllModules::getAllModules()->getLogFp();
    fprintf(logFp, "  Line %d: %s\n", lineNum, successStr);
    fflush(logFp);
    }


int main()
    {
    TestAllModules::getAllModules()->runAllTests();
    }
