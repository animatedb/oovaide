/*
 * File.h
 *
 *  Created on: Oct 14, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>
#include "OovString.h"
#define __NO_MINGW_LFS 1

/// This is a simple buffered file interface.
class File
    {
    public:
        File():
            mFp(nullptr)
            {}
        File(OovStringRef const fn, OovStringRef const mode)
            {
            mFp = fopen(fn, mode);
            }
        ~File()
            { close(); }
        void open(OovStringRef const fn, OovStringRef const mode)
            {
            if(!mFp)
                mFp = fopen(fn, mode);
            }
        void close()
            {
            if(mFp)
                fclose(mFp);
            mFp = nullptr;
            }
        void truncate(int size=0);
        bool isOpen() const
            { return(mFp != nullptr); }
        FILE *getFp()
            { return mFp; }
    private:
        FILE *mFp;
    };

enum eOpenModes { M_ReadShared, M_ReadWriteExclusive, M_ReadWriteExclusiveAppend,
    M_WriteExclusiveTrunc };
enum eOpenEndings { OE_Text, OE_Binary };
enum eOpenStatus { OS_Opened, OS_SharingProblem, OS_NoFile, OS_OtherError };

class BaseSimpleFile
    {
    public:
        BaseSimpleFile():
            mFd(-1)
            {}
        ~BaseSimpleFile()
            { close(); }
        eOpenStatus open(OovStringRef const fn=nullptr, eOpenModes mode=M_ReadWriteExclusive,
                eOpenEndings oe=OE_Text);
        void close();
        bool isOpen() const
            { return(mFd != -1); }
        // actual size may be less than file size in text mode.
        bool read(void *buf, int size, int &actualSize);
        bool write(void const *buf, int size);
        void seekBegin();
        int getSize() const;
        void setFd(int fd)
            { mFd = fd; }
        int getFd()
            { return mFd; }
        void truncate(int size=0);

    protected:
        int mFd;
    };

/// non-buffered file
class SimpleFile:public BaseSimpleFile
    {
    public:
        SimpleFile(OovStringRef const fn=nullptr, eOpenModes mode=M_ReadWriteExclusive,
                eOpenEndings oe=OE_Text)
            {
            if(fn)
                open(fn, mode, oe);
            }
    };

/// This defines a shared file between processes.  This will prevent writing
/// the same file from multiple processes at a time.  The open will delay
/// if the file cannot be opened exclusively when the open flag indicates
/// exclusive access is needed.
class SharedFile: public BaseSimpleFile
    {
    public:
        eOpenStatus open(OovStringRef const fn, eOpenModes mode,
                eOpenEndings oe=OE_Text);
    };

#endif /* FILE_H_ */
