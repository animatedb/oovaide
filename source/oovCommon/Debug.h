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
        /// Use the debug file interface around the file handle.
        DebugFile(FILE *fp):
            mOwnFile(false)
            { mFp = fp; }
        /// Create a debug file with name.
        /// @param fn must not be OovStringRef for static strings since std::string
        ///     has not been initialized yet.
        /// @param append Set true to append, false to truncate
        DebugFile(char const *fn, bool append);
        ~DebugFile();
        /// Print and flush to file
        /// @param format The format specifier.
        void printflush(OovStringRef const format, ...);
        /// Open a file
        /// @param fn The file path
        /// @param mode The fopen mode.
        void open(OovStringRef const fn, OovStringRef const mode = "w")
            {
            if(!mFp)
                mFp = fopen(fn, mode);
            }
        /// Check if the file is open.
        bool isOpen() const
            { return(mFp != nullptr); }
        /// Get the file pointer of the file
        FILE *getFp() const
            { return mFp; }
    public:
        FILE *mFp;
        bool mOwnFile;
    };

void LogAssertFile(OovStringRef const file, int line, char const *diagStr=nullptr);

#define DebugAssert(file, line)         LogAssertFile(file, line);

#endif /* DEBUG_H_ */
