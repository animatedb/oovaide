/*
 * File.h
 *
 *  Created on: Oct 14, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef FILE_H_
#define FILE_H_

#include "OovString.h"
#include "OovError.h"
#include <stdio.h>
#include <sys/stat.h>
#define __NO_MINGW_LFS 1


// This is stuck here because we have no better place at this TIME.
void sleepMs(int ms);


/// This is a simple buffered file interface.
class File
    {
    public:
        File():
            mFp(nullptr)
            {}
        ~File()
            { close(); }

        /// Open a file
        /// @param fn The name of the file to open
        /// @param mode The fopen mode
        OovStatusReturn open(OovStringRef const fn, OovStringRef const mode)
            {
            OovStatus status(true, SC_File);
            if(!mFp)
                {
                mFp = fopen(fn, mode);
                status.set(mFp != nullptr, SC_File);
                }
            return status;
            }

        /// Close the file. The destructor will also ensure the file is closed.
        void close()
            {
            if(mFp)
                fclose(mFp);
            mFp = nullptr;
            }

        /// Check if a file is open.
        bool isOpen() const
            { return(mFp != nullptr); }

        /// Gets the size of the file.
        /// @param size The returned size.
        OovStatusReturn getFileSize(int &size) const;

        /// Reads the file into a buffer at the current seek position.
        /// @param buf The buffer to fill
        /// @param size The number of bytes to read.
        OovStatusReturn read(char *buf, int size) const
            {
            size_t actualSize = fread(buf, 1, size, mFp);
            return(OovStatus(ferror(mFp) == 0 && actualSize > 0, SC_File));
            }

        /// Writes the file into a buffer at the current seek position.
        /// @param buf The buffer to read from
        /// @param size The number of bytes to write.
        OovStatusReturn write(char const *buf, int size) const
            {
            size_t writeSize = fwrite(buf, 1, size, mFp);
            return(OovStatus(writeSize == static_cast<size_t>(size), SC_File));
            }

        /// Write a string. This does not append an end of line indicator.
        /// @param str The string to write.
        OovStatusReturn putString(OovStringRef const &str) const
            {
            return(OovStatus(fputs(str.getStr(), mFp) >= 0, SC_File));
            }

        /// Read a string up to the end of line.
        /// @param buf The place to put the string. The size must be greater or
        //      equal to bufBytes
        /// @param bufBytes the maximum number of bytes to read.
        /// @param status True if no error was encountered.
        bool getString(char *buf, int bufBytes, OovStatus &status);

        /// Set a file to the specified size.  This is meant for truncation and
        /// not for growing a file.
        /// @param size The size in bytes to set the file to.
        void truncate(int size=0);

        /// Seek to the beginning of the file
        OovStatus seekBegin() const
            { return(OovStatus(fseek(mFp, 0, SEEK_SET) == 0, SC_File)); }

        /// Seek to the end of the file
        OovStatusReturn seekEnd() const
            { return(OovStatus(fseek(mFp, 0, SEEK_END) == 0, SC_File)); }

        long getFileSize() const
            { return ftell(mFp); }

        OovStatusReturn getModifyTime(time_t &modifyTime) const
            {
            struct stat buf;
            bool success = (fstat(fileno(mFp), &buf) == 0);
            modifyTime = buf.st_mtime;
            return OovStatus(success, SC_File);
            }

        /// Get the file pointer of the open file. Null if not opened.
        FILE *getFp()
            { return mFp; }

    private:
        FILE *mFp;
    };

enum eOpenModes { M_ReadShared, M_ReadWriteExclusive, M_ReadWriteExclusiveAppend,
    M_WriteExclusiveTrunc };
enum eOpenEndings { OE_Text, OE_Binary };
enum eOpenStatus { OS_Opened, OS_SharingProblem, OS_NoFile, OS_OtherError };

/// This is a simple non-buffered file interface.
class BaseSimpleFile
    {
    public:
        BaseSimpleFile():
            mFd(-1)
            {}
        ~BaseSimpleFile()
            { close(); }
        /// Open a file.
        /// @param fn The name of the file to open
        /// @param mode The open mode
        /// @param oe The open endings (text or binary)
        eOpenStatus open(OovStringRef const fn=nullptr, eOpenModes mode=M_ReadWriteExclusive,
                eOpenEndings oe=OE_Text);
        /// Close the file
        void close();
        /// Check if the file is open
        bool isOpen() const
            { return(mFd != -1); }
        /// Read bytes from a file.
        /// @param buf The place to read the data into.
        /// @param size The number of bytes to read.
        /// @param actualSize The read number of bytes.  The actual size may
        ///     be less than file size in text mode.
        OovStatusReturn read(void *buf, int size, int &actualSize);
        /// Write bytes to a file.
        /// @param buf The data to write
        /// @param size The number of bytes to write
        OovStatusReturn write(void const *buf, int size);
        /// Seek to the beginning of the file
        OovStatusReturn seekBegin();
        /// Get the size of the file in bytes
        int getSize() const;
        /// Set the file descriptor
        /// @param fd The file descriptor
// DEAD CODE
//        void setFd(int fd)
//            { mFd = fd; }
        /// Get the file descriptor
// DEAD CODE
//        int getFd()
//            { return mFd; }
        /// Truncate the file to a smaller size.
        /// @param the size of the file to set to.
        void truncate(int size=0);

    protected:
        int mFd;
    };

/// This is a simple non-buffered file.
class SimpleFile:public BaseSimpleFile
    {
    public:
        /// Open a file.
        /// @param fn The name of the file to open
        /// @param mode The open mode
        /// @param oe The open endings (text or binary)
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
        /// Open a file.
        /// @param fn The name of the file to open
        /// @param mode The open mode
        /// @param oe The open endings (text or binary)
        eOpenStatus open(OovStringRef const fn, eOpenModes mode,
                eOpenEndings oe=OE_Text);
    };

#endif /* FILE_H_ */
