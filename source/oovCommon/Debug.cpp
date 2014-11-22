/*
 * Debug.cpp
 *
 *  Created on: Nov 19, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "Debug.h"

DebugFile gDebugAssertFile("OovAsserts.txt", true);

void LogAssertFile(char const *file, int line)
    {
    gDebugAssertFile.printflush("%s %d\n", file, line);
    }

DebugFile::DebugFile(char const * const fn, bool append):
    mOwnFile(true)
    {
    if(fn)
	{
	if(append)
	    mFp = fopen(fn, "a");
	else
	    mFp = fopen(fn, "w");
	}
    }

DebugFile::~DebugFile()
    {
    if(mFp)
	fclose(mFp);
    mFp = nullptr;
    }

void DebugFile::printflush(char const * const format, ...)
    {
    if(mFp)
	{
	va_list argList;

	va_start(argList, format);
	vfprintf(mFp, format, argList);
	va_end(argList);
	fflush(mFp);
	}
    }