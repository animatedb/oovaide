/*
 * Debug.h
 *
 *  Created on: Jun 28, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdio.h>
#include <stdarg.h>
#include "OovString.h"

/// This class is just used to aid debugging.
class DebugFile
    {
    public:
	DebugFile(FILE *fp):
            mOwnFile(false)
            { mFp = fp; }
	DebugFile(OovStringRef const fn, bool append);
	~DebugFile();
	void printflush(OovStringRef const format, ...);
	void open(OovStringRef const fn, OovStringRef const mode = "w")
	    {
	    if(!mFp)
		mFp = fopen(fn, mode);
	    }
	bool isOpen() const
	    { return(mFp != nullptr); }
	FILE *getFp() const
	    { return mFp; }
    public:
	FILE *mFp;
        bool mOwnFile;
    };

void LogAssertFile(OovStringRef const file, int line);

#define DebugAssert(file, line)		LogAssertFile(file, line);

#endif /* DEBUG_H_ */
