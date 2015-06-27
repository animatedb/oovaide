/*
 * File.cpp
 *
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */
#include "File.h"
#include "FilePath.h"
#include "Debug.h"
//#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/file.h>   // for flock
#else
#include <share.h>
#include <io.h>         // For _sopen_s - in Windows, mingw-builds is required.
#endif
#include <errno.h>


// Can't include OovProcess because some projects don't use glib.
#ifdef __linux__
#include <unistd.h>     // for usleep
#else
#include <windows.h>    // for Sleep
#endif
static void sleepMs(int ms)
    {
#ifdef __linux__
    usleep(ms*1000);
#else
    Sleep(ms);
#endif
    }


void File::truncate(int size)
    {
#ifdef __linux__
    if(ftruncate(fileno(mFp), size) != 0)
        {
        DebugAssert(__FILE__, __LINE__);
        }
#else
    _chsize(fileno(mFp), size);
#endif
    }


eOpenStatus BaseSimpleFile::open(OovStringRef const fn, eOpenModes mode, eOpenEndings oe)
    {
    eOpenStatus status = OS_SharingProblem;
#ifdef __linux__
    int lockStat;
    int flags;
    if(mode == M_ReadShared)
        {
        flags = O_RDONLY;
        }
    else
        {
        flags = O_CREAT | O_RDWR;
        if(mode == M_ReadWriteExclusiveAppend)
            flags |= O_APPEND;
        }
    // Set permissions for new file so that user/group/others have access for read/write/exec.
    mFd = ::open(fn, flags, S_IRWXU | S_IRWXG | S_IRWXO);
    if(mFd != -1)
        lockStat = flock(mFd, LOCK_EX);
    if(mFd == -1)
        {
        status = OS_NoFile;
        }
    else if(lockStat == 0)
        {
        status = OS_Opened;
        }
#else

    int openFlags = 0;
    int sharedFlags = 0;
    int permissionFlags = _S_IREAD | _S_IWRITE;
    if(mode == M_ReadShared)
        {
        openFlags = _O_RDONLY;
        sharedFlags = _SH_DENYWR;
        }
    else if(mode == M_WriteExclusiveTrunc)
        {
        openFlags = _O_CREAT | _O_WRONLY | _O_TRUNC;
        sharedFlags = _SH_DENYRW;
        }
    else
        {
        // Creates if it doesn't exist.
        openFlags = _O_CREAT | _O_RDWR;
        sharedFlags = _SH_DENYRW;
        if(mode == M_ReadWriteExclusiveAppend)
            openFlags |= _O_APPEND;
        }
    if(oe == OE_Text)
        openFlags |= _O_TEXT;
    else
        openFlags |= _O_BINARY;
    _sopen_s(&mFd, fn, openFlags, sharedFlags, permissionFlags);
    if(mFd == -1)
        {
        switch(errno)
            {
            case ENOENT:        status = OS_NoFile;             break;
//          case EACCES:        status = OS_SharingProblem;     break;
//          default:            status = OS_OtherError;         break;
            default:            status = OS_SharingProblem;     break;
            }
        }
    else
        {
        status = OS_Opened;
        }
#endif
    return status;
    }

bool BaseSimpleFile::read(void *buf, int size, int &actualSize)
    {
#ifdef __linux__
    actualSize = ::read(mFd, buf, size);
#else
    actualSize = _read(mFd, buf, size);
#endif
    // actualSize may be less than size in text mode
    return(actualSize >= 0);
    }

bool BaseSimpleFile::write(void const *buf, int size)
    {
    int bytesWritten;
#ifdef __linux__
    bytesWritten = ::write(mFd, buf, size);
#else
    bytesWritten = _write(mFd, buf, size);
#endif
    return(bytesWritten == size);
    }

int BaseSimpleFile::getSize() const
    {
    struct OovStat32 filestat;
    OovFStatFunc(mFd, &filestat);
    return filestat.st_size;
    }

void BaseSimpleFile::seekBegin()
    {
#ifdef __linux__
    lseek(mFd, 0, SEEK_SET);
#else
    _lseek(mFd, 0, SEEK_SET);
#endif
    }

void BaseSimpleFile::close()
    {
#ifdef __linux__
    ::close(mFd);
#else
    _close(mFd);
#endif
    }

void BaseSimpleFile::truncate(int size)
    {
#ifdef __linux__
    if(ftruncate(mFd, size) != 0)
        {
        DebugAssert(__FILE__, __LINE__);
        }
#else
    _chsize(mFd, size);
#endif
    }

eOpenStatus SharedFile::open(OovStringRef const fn, eOpenModes mode,
        eOpenEndings oe)
    {
    eOpenStatus status = OS_SharingProblem;
    for(int i=0; i<50 && status == OS_SharingProblem; i++)
        {
        status = BaseSimpleFile::open(fn, mode, oe);
        if(status == OS_SharingProblem)
            sleepMs(100);
        }
    return status;
    }
