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

cTestAllModules *cTestAllModules::sTestAllModules;

cTestAllModules::cTestAllModules()
    {
    mLogFp = fopen("TestCpp.txt", "w");
    }

cTestAllModules::~cTestAllModules()
    {
    fclose(mLogFp);
    }

void cTestAllModules::addModule(class cTestCppModule *module)
    {
    if(!cTestAllModules::sTestAllModules)
        cTestAllModules::sTestAllModules = new cTestAllModules();
    cTestAllModules::sTestAllModules->mModules.push_back(module);
    }

void cTestAllModules::runAllTests()
    {
    for(auto const &mod : mModules)
        {
	fprintf(mLogFp, "Module %s\n", mod->getModuleName());
        mod->runAllTests();
        }
    }

///////////////

cTestCppModule::cTestCppModule(char const * const moduleName):
	mPassCount(0), mFailCount(0), mModuleName(moduleName)
	{
        cTestAllModules::addModule(this);
	}

void cTestCppModule::addFunctionTest(class cTestCppFunction *func)
	{ mTestFunctions.push_back(func); }

void cTestCppModule::runAllTests()
	{
        FILE *logFp = cTestAllModules::getAllModules()->getLogFp();
	for(size_t i=0; i<mTestFunctions.size(); i++)
		{
		cTestCppFunction &testFunc = *mTestFunctions[i];
		fprintf(logFp, "%d:Function %s %s\n",
                    i+1, testFunc.mModuleName.c_str(), testFunc.mTestName.c_str());
		testFunc.doFunctionTest(*this);
		if(testFunc.mAnyFailure)
			mFailCount++;
		else
			mPassCount++;
		}
	if(mExtraDiagnostics.size() > 0)
	    {
	    fprintf(logFp, "\nExtra Diagnostics\n");
	    for(size_t i=0; i<mExtraDiagnostics.size(); i++)
		{
		fprintf(logFp, "  %s\n", mExtraDiagnostics[i].c_str());
		}
	    }
	fprintf(logFp, "\nSummary:  %d Passed, %d Failed\n\n",
            mPassCount, mFailCount);
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
    fprintf(cTestAllModules::getAllModules()->getLogFp(),
        "  Line %d: %s\n", lineNum, successStr);
    }


int main()
    {
    cTestAllModules::getAllModules()->runAllTests();
    }
