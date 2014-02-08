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
	    {
	    if(mFp)
		fclose(mFp);
	    }
	void open(char const * const fn, char const * const mode)
	    {
	    if(!mFp)
		mFp = fopen(fn, mode);
	    }
	FILE *getFp()
	    { return mFp; }
    private:
	FILE *mFp;
    };

#endif /* FILE_H_ */
