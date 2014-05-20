/*
 * File.cpp
 *
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "File.h"
#include "FilePath.h"
//#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/file.h>	// for flock
#else
#include <share.h>
#include <io.h>		// For _sopen_s - in Windows, mingw-builds is required.
#endif
#include <errno.h>
#include <assert.h>


// Can't include OovProcess because some projects don't use glib.
#ifdef __linux__
#include <unistd.h>	// for usleep
#else
#include <windows.h>	// for Sleep
#endif
static void sleepMs(int ms)
    {
#ifdef __linux__
    usleep(ms*1000);
#else
    Sleep(ms);
#endif
    }


SharedFile::eOpenStatus SharedFile::openFile(char const * const fn, eModes mode)
    {
    eOpenStatus status = OS_SharingProblem;
    for(int i=0; i<50 && status == OS_SharingProblem; i++)
	{
#ifdef __linux__
	int lockStat;
	if(mode == M_ReadShared)
	    {
	    mFd = open(fn, O_RDONLY);
	    if(mFd != -1)
		lockStat = flock(mFd, LOCK_SH);
	    }
	else
	    {
	    mFd = open(fn, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	    if(mFd != -1)
		lockStat = flock(mFd, LOCK_EX);
	    }
	if(mFd == -1)
	    {
	    status = OS_NoFile;
	    }
	else if(lockStat == 0)
	    {
	    status = OS_Opened;
	    }
#else
	if(mode == M_ReadShared)
	    {
	    _sopen_s(&mFd, fn, _O_RDONLY, _SH_DENYWR, _S_IREAD | _S_IWRITE);
	    }
	else
	    {
	    // Creates if it doesn't exist.
	    _sopen_s(&mFd, fn, _O_CREAT | _O_RDWR, _SH_DENYRW, _S_IREAD | _S_IWRITE);
	    }
	if(mFd == -1)
	    {
	    switch(errno)
		{
		case ENOENT:	status = OS_NoFile;		break;
//		case EACCES:	status = OS_SharingProblem;	break;
//		default:	status = OS_OtherError;		break;
		default:	status = OS_SharingProblem;	break;
		}
	    }
	else
	    {
	    status = OS_Opened;
	    }
#endif
	if(status == OS_SharingProblem)
	    sleepMs(100);
	}
    return status;
    }

bool SharedFile::readFile(void *buf, int size, int &actualSize)
    {
#ifdef __linux__
    actualSize = read(mFd, buf, size);
#else
    actualSize = _read(mFd, buf, size);
#endif
    // actualSize may be less than size in text mode
    return(actualSize >= 0);
    }

int SharedFile::getFileSize() const
    {
    struct OovStat32 filestat;
    OovFStatFunc(mFd, &filestat);
    return filestat.st_size;
    }

void SharedFile::seekBeginFile()
    {
#ifdef __linux__
    lseek(mFd, 0, SEEK_SET);
#else
    _lseek(mFd, 0, SEEK_SET);
#endif
    }

bool SharedFile::writeFile(void const *buf, int size)
    {
    int bytesWritten;
#ifdef __linux__
    bytesWritten = write(mFd, buf, size);
#else
    bytesWritten = _write(mFd, buf, size);
#endif
    return(bytesWritten == size);
    }

void SharedFile::closeFile()
    {
#ifdef __linux__
    close(mFd);
#else
    _close(mFd);
#endif
    }
