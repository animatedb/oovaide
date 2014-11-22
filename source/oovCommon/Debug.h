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

/// This class is just used to aid debugging.
class DebugFile
    {
    public:
	DebugFile(FILE *fp):
            mOwnFile(false)
            { mFp = fp; }
	DebugFile(char const * const fn=nullptr, bool append=false);
	~DebugFile();
	void printflush(char const * const format, ...);
	void open(char const * const fn, char const * const mode = "w")
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

void LogAssertFile(char const *file, int line);

#define DebugAssert(file, line)		LogAssertFile(file, line);

#endif /* DEBUG_H_ */
