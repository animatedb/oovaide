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


void cTime::sleepMs(int ms)
{
#ifndef _X86_
	usleep(ms*1000);
#else
	Sleep(ms);
#endif
}

void cTime::getCurrentTime()
    {
    gettimeofday(&mTime, NULL);
    }

double cTime::elapsedSecondsSinceStart(const cTime &startTime)
    {
    return(cTime(mTime) - startTime);
    }


cTestCppModule::cTestCppModule():
	mPassCount(0), mFailCount(0)
	{
	mLogFp = fopen("TestCpp.txt", "w");
	}

void cTestCppModule::addFunctionTest(class cTestCppFunction *func)
	{ mTestFunctions.push_back(func); }

void cTestCppModule::runAllTests()
	{
	for(size_t i=0; i<mTestFunctions.size(); i++)
		{
		cTestCppFunction &testFunc = *mTestFunctions[i];
		fprintf(mLogFp, "%d:Function %s %s\n", i+1, testFunc.mModuleName.c_str(), testFunc.mTestName.c_str());
		testFunc.doFunctionTest(*this);
		if(testFunc.mAnyFailure)
			mFailCount++;
		else
			mPassCount++;
		}
	if(mExtraDiagnostics.size() > 0)
	    {
	    fprintf(mLogFp, "\nExtra Diagnostics\n");
	    for(size_t i=0; i<mExtraDiagnostics.size(); i++)
		{
		fprintf(mLogFp, "  %s\n", mExtraDiagnostics[i].c_str());
		}
	    }
	fprintf(mLogFp, "\nSummary:  %d Passed, %d Failed\n", mPassCount, mFailCount);
	}

void cTestCppModule::addExtraDiagnostics(const char *str, double val)
    {
    char buf[100];
    sprintf(buf, "%s: %f", str, val);
    addExtraDiagnostics(buf);
    }

//////

void cTestCppFunction::testCppOutput(int lineNum, bool passed)
{
	const char *successStr;
	if(passed)
		successStr = "Pass";
	else
		{
		successStr = "Fail";
		mAnyFailure = true;
		}
	fprintf(mParentModule.getLogFp(), "  Line %d: %s\n", lineNum, successStr);
}

