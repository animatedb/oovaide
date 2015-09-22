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


// This is stuck here because we have no better place at this TIME.
void sleepMs(int ms);


/// This is a simple buffered file interface.
class File
    {
    public:
        File():
            mFp(nullptr)
            {}

        /// Open a file
        /// @param fn The name of the file to open
        /// @param mode The fopen mode
        File(OovStringRef const fn, OovStringRef const mode)
            {
            mFp = fopen(fn, mode);
            }
        ~File()
            { close(); }

        /// Open a file
        /// @param fn The name of the file to open
        /// @param mode The fopen mode
        void open(OovStringRef const fn, OovStringRef const mode)
            {
            if(!mFp)
                mFp = fopen(fn, mode);
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
        bool getFileSize(int &size) const;

        /// Reads the file into a buffer at the current seek position.
        /// @param buf The buffer to fill
        /// @param size The number of bytes to read.
        bool read(char *buf, int size) const
            {
            size_t actualSize = fread(buf, 1, size, mFp);
            return(ferror(mFp) == 0 && actualSize > 0);
            }

        /// Write a string. This does not append an end of line indicator.
        /// @param str The string to write.
        bool putString(OovStringRef const &str) const
            {
            return(fputs(str.getStr(), mFp) >= 0);
            }

        /// Read a string up to the end of line.
        /// @param buf The place to put the string. The size must be greater or
        //      equal to bufBytes
        /// @param bufBytes the maximum number of bytes to read.
        /// @param success True if no error was encountered.
        bool getString(char *buf, int bufBytes, bool &success);

        /// Set a file to the specified size.  This is meant for truncation and
        /// not for growing a file.
        /// @param size The size in bytes to set the file to.
        void truncate(int size=0);

        /// Seek to the beginning of the file
        bool seekBegin() const
            { return(fseek(mFp, 0, SEEK_SET) == 0); }

        /// Seek to the end of the file
        bool seekEnd() const
            { return(fseek(mFp, 0, SEEK_END) == 0); }

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
        bool read(void *buf, int size, int &actualSize);
        /// Write bytes to a file.
        /// @param buf The data to write
        /// @param size The number of bytes to write
        bool write(void const *buf, int size);
        /// Seek to the beginning of the file
        void seekBegin();
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
