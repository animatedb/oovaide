/*
 * Debug.cpp
 *
 *  Created on: Nov 19, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "Debug.h"
#include <string>


void LogAssertFile(OovStringRef const file, int line, char const *diagStr)
    {
    DebugFile debugAssertFile("OovAsserts.txt", true);
    std::string str;
    if(diagStr)
        {
        str = ' ';
        str += diagStr;
        }
    debugAssertFile.printflush("%s %d%s\n", file.getStr(), line, diagStr);
    }

DebugFile::DebugFile(char const *fn, bool append):
    mOwnFile(true)
    {
    if(fn)
        {
        mFp = fopen(fn, "rb");
        if(mFp)
            {
            fseek(mFp, 0, SEEK_END);
            int size = ftell(mFp);
            if(size > 100000)
                {
                append = false;
                fclose(mFp);
                }
            }
        if(append)
            {
            mFp = fopen(fn, "a");
            }
        else
            {
            mFp = fopen(fn, "w");
            }
        }
    }

DebugFile::~DebugFile()
    {
    if(mFp)
        fclose(mFp);
    mFp = nullptr;
    }

void DebugFile::printflush(OovStringRef const format, ...)
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
