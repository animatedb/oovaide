/*
 * Debug.cpp
 *
 *  Created on: Nov 19, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "Debug.h"
#include <assert.h>
#include <string>
#include "OovError.h"


void LogAssertFile(OovStringRef const file, int line, char const *diagStr)
    {
    OovString errStr = "ASSERT: ";
    errStr += file;
    errStr += ' ';
    errStr.appendInt(line);
    if(diagStr)
        {
        errStr += ' ';
        errStr += diagStr;
        }
    OovError::report(ET_Diagnostic, errStr);
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
