/*
 * File.h
 *
 *  Created on: Oct 14, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef FILE_H_
#define FILE_H_

#include "stdio.h"

class File
    {
    public:
	File(char const * const fn=nullptr, char const * const mode=nullptr):
	    mFp(NULL)
	    {
	    if(fn)
		mFp = fopen(fn, mode);
	    }
	~File()
	    { close(); }
	void open(char const * const fn, char const * const mode)
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
        bool isOpen() const
            { return(mFp != nullptr); }
	FILE *getFp()
	    { return mFp; }
    private:
	FILE *mFp;
    };

class SharedFile
    {
    public:
	~SharedFile()
	    { closeFile(); }
	enum eModes { M_ReadShared, M_ReadWriteExclusive };
	enum eOpenStatus { OS_Opened, OS_SharingProblem, OS_NoFile, OS_OtherError };
	eOpenStatus openFile(char const * const fn, eModes mode);
	int getFileSize() const;
	void seekBeginFile();
	// actual size may be less than file size in text mode.
	bool readFile(void *buf, int size, int &actualSize);
	bool writeFile(void const *buf, int size);
	void closeFile();

    private:
	int mFd;
    };

#endif /* FILE_H_ */
