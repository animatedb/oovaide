/*
 * FilePath.h
 *
 *  Created on: Oct 13, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

// The name "path" means either a directory, filename, etc. or whatever.
// A path can contain a device or drive, directory, filename and extension

#ifndef FILEPATH_H_
#define FILEPATH_H_

#include "OovString.h"
#include "OovError.h"
#include <sys/stat.h>
#include <vector>

#ifdef __linux__
#define OovStat32 stat
#define OovStatFunc stat
#define OovFStatFunc fstat
#else
#if(__MINGW_MAJOR_VERSION >= 4)
#define OovStat32 _stat64i32
#define OovStatFunc _stat
#define OovFStatFunc _fstat
#else
#define OovStat32 stat
#define OovStatFunc stat
#define OovFStatFunc fstat
#endif
#endif

// A path is made up of optional parts.
//      The parts are drive, directory segments..., filename, extension.
// Example: "c:/dirseg1/dirseg2/name.ext"
// A drive spec is considered to be anything up to the last colon.

// See FilePath for rules on the paths when inside of a FilePath class.

enum eReturnPosition {
    RP_RetPosNatural,   // Return the most natural position in the path.
    RP_RetPosFailure    // Return std::string::npos if the position is not in the path.
};

enum eFilePathTypes { FP_File, FP_Dir, FP_Ext };


//***** non-modifying path functions
/// Get the start of the directory specification. This is the position after
/// after the drive spec. If there is no drive spec, this returns the
/// beginning (0).
/// @param path A file path.
size_t FilePathGetPosStartDir(OovStringRef const path);

/// Get the position at the end of the directory specification. This is
/// the position of the last path separator. If there are no path separators,
/// this returns the start dir pos.
/// @param path A file path.
size_t FilePathGetPosEndDir(OovStringRef const path);

/// Get the position of the extension.  This is the position of the
/// extension separator. Since a period may be used for relative path specs,
/// a check is made to see if the period is after a path separator, and only
/// then is it considered an extension. If the extension is not present,
/// the last character position is returned.
/// @param path A file path.
size_t FilePathGetPosExtension(OovStringRef const path, eReturnPosition rp);

/// Get the position of a directory segment.  This is the text between
/// path separators.  Only case insensitive comparisons are done on Windows.
/// Returns std::string::npos if the segment is not found.
//// @todo - the following should probably be changed.
/// The position returned is pointing to the segment. (If segment starts
/// with a path separator, the position will contains the path separator)
/// @param path A file path.
/// @param seg Some text that does not contain any path separators.
size_t FilePathGetPosSegment(OovStringRef const path, OovStringRef const seg);

/// Get the position of a path separator to the left of the passed in position.
/// If using RP_RetPosNatural, and there is no left path separator, the
/// beginning (0) is returned.
/// @param path A file path.
/// @param pos The reference position.
/// @param rp The returned position specification.
size_t FilePathGetPosLeftPathSep(OovStringRef const path, size_t pos, eReturnPosition rp);

/// Get the position of a path separator to the right of the passed in position.
/// If using RP_RetPosNatural, and there is no right path separator, the
/// beginning (0) is returned.
/// @param path A file path.
/// @param pos The reference position.
/// @param rp The returned position specification.
size_t FilePathGetPosRightPathSep(OovStringRef const path, size_t pos, eReturnPosition rp);

/// Check if the path is an absolute path.
/// If the first character is a path separator or if a drive spec is present
/// with a path separator after it, then the path is considered to be absolute.
/// @param path The file path to check.
bool FilePathIsAbsolutePath(OovStringRef const path);

/// Check if the character at the specified position is a path separator.
/// @param path The file path to check.
/// @param pos The position to test.
bool FilePathIsPathSep(OovStringRef const path, size_t pos);

/// Check if the path ends with a path separator.
/// @param path The file path to check.
bool FilePathIsEndPathSep(OovStringRef const path);

/// Check if the character at the specified position is an extension separator.
/// @param path The file path to check.
/// @param pos The position to test.
bool FilePathIsExtensionSep(OovStringRef const path, size_t pos);

/// Check if the path has an extension.
/// @param path The file path to check.
bool FilePathHasExtension(OovStringRef const path);

/// See if the paths passed in have matching extensions.
/// @param path1 The file path to check.
/// @param path2 The file path to check.
bool FilePathMatchExtension(OovStringRef const path1, OovStringRef const path2);

/// See if the paths passed in have matching extensions.
/// In Windows, this is a case insensitive comparison.
/// @param path1 The file path to check.
/// @param path2 The file path to check.
int FilePathComparePaths(OovStringRef const path1, OovStringRef const path2);

/// Get the string to the left and including the position. A position of
/// zero with a path without a drive or path spec returns an empty string.
/// @param path A file path.
/// @param pos The reference position.
OovString FilePathGetHead(OovStringRef const path, size_t pos);

/// Get the string to the right and including the position.
/// @param path A file path.
/// @param pos The reference position.
OovString FilePathGetTail(OovStringRef const path, size_t pos);

/// Gets the segment that encloses the position. The segment is returned
/// without path separators. The position can be pointing to the anywhere
/// within the desired segment.
/// If it is pointing to a path separator, the segment following the path
/// separator is returned.
/// If the segment is not found, this returns std::string::npos.
/// @param path A file path.
/// @param pos The reference position.
OovString FilePathGetSegment(OovStringRef const path, size_t pos);

/// Gets the drive and directory specs of the path.
/// @param path A file path.
OovString FilePathGetDrivePath(OovStringRef const path);

/// Gets the filename without the extension of the path.
/// @param path A file path.
OovString FilePathGetFileName(OovStringRef const path);

/// Gets the filename and extension of the path.
/// @param path A file path.
OovString FilePathGetFileNameExt(OovStringRef const path);

/// Gets the extension of the path.
/// @param path A file path.
OovString FilePathGetFileExt(OovStringRef const path);

/// Gets the drive and path without the end separator.
/// @param path A file path.
OovString FilePathGetWithoutEndPathSep(OovStringRef const path);

/// Gets the path where the directory separators are converted to a
/// foward slash character.
#ifndef __linux__
OovString FilePathGetAsWindowsPath(OovStringRef const path);
#endif

//***** modifying path functions

/// Appends a path separator if the path does not already end with one.
/// @param path The path to modify.
void FilePathEnsureLastPathSep(std::string &path);

/// Remove the path separator at the specified position if it is a path
/// separator.
/// @param path The path to modify.
void FilePathRemovePathSep(std::string &path, size_t pos);

/// If the path does not start with a double quote, then add quotes to
/// the beginning and end.
/// @param path The path to modify.
void FilePathQuoteCommandLinePath(std::string &path);

/// Append the proper extension for a file that is an executable depending on
/// the operating system.
/// @param path The path to modify.
OovString FilePathMakeExeFilename(OovStringRef const rootFn);

/// Remove consecutive directory separators.  It appears that GTK, GDB or Eclipse
/// adds them at some point. For some reason, oovEdit requires this?
/// @param path The path to modify.
std::string FilePathFixFilePath(OovStringRef const path);

//***** File/disk operations

/// If the path does not exist, create all of the subdirectories required
/// to match the path.  The path must be a directory.
/// @param path The path to use to build the directories.
OovStatusReturn FileEnsurePathExists(OovStringRef const path);

/// Check if the specified file is on disk
/// @param path The path to check.
/// @param status True if the check was performed successfully.
bool FileIsFileOnDisk(OovStringRef const path, OovStatus &status);

/// Check if the specified directory is on disk
/// @param path The path to check.
/// @param status True if the check was performed successfully.
bool FileIsDirOnDisk(OovStringRef const path, OovStatus &status);

OovStatusReturn FileMakeSubDir(OovStringRef partPath);

/// Get the modify time of the file.
/// This does return an error if the file does not exist.
/// @param path The path to use to get the time.
/// @param time The returned time of the file.
OovStatusReturn FileGetFileTime(OovStringRef const path, time_t &time);

/// Delete the specified file.
/// @param path The file to delete.
OovStatusReturn FileDelete(OovStringRef const path);

/// Wait for a directory to be deleted.
/// Windows can return from a delete file or directory call before it
/// is finished being deleted.  It may be related to TortoiseSvn keeping
/// files open.
/// @param path The file to wait for deletion.
OovStatusReturn FileWaitForDirDeleted(OovStringRef const path, int waitMs=10000);

/// Rename a file
/// @param oldPath The original path name.
/// @param newPath The new path name.
OovStatusReturn FileRename(OovStringRef const oldPath, OovStringRef const newPath);

class FileStat
    {
    public:
        /// This does not return an error if the file does not exist. It just
        /// indicates that the file is old.
        static bool isOutputOld(OovStringRef const outputFn,
            OovStringRef const inputFn, OovStatus &status);
        /// This does not return an error if the file does not exist. It just
        /// indicates that the file is old.
        static bool isOutputOld(OovStringRef const outputFn,
            OovStringVec const &inputs, OovStatus &status, size_t *oldIndex=nullptr);
    };

template<typename T_Str> class FilePathRefInterface
    {
    public:
        size_t getPosStartDir() const
            { return FilePathGetPosStartDir(getThisStr()); }
        size_t getPosEndDir() const
            { return FilePathGetPosEndDir(getThisStr()); }
        size_t getPosExtension(eReturnPosition rp) const
            { return FilePathGetPosExtension(getThisStr(), rp); }
        size_t getPosPathSegment(OovStringRef const seg) const
            { return FilePathGetPosSegment(getThisStr(), seg); }
        size_t getPosLeftPathSep(size_t startPos, eReturnPosition rp) const
            { return FilePathGetPosLeftPathSep(getThisStr(), startPos, rp); }
        size_t getPosRightPathSep(size_t startPos, eReturnPosition rp) const
            { return FilePathGetPosRightPathSep(getThisStr(), startPos, rp); }

        OovString getHead(size_t startPos) const
            { return FilePathGetHead(getThisStr(), startPos); }
        OovString getTail(size_t startPos) const
            { return FilePathGetTail(getThisStr(), startPos); }
        // Return a part of the path, (not the full tail)
        OovString getPathSegment(size_t startPos) const
            { return FilePathGetSegment(getThisStr(), startPos); }
        OovString getDrivePath() const
            { return FilePathGetDrivePath(getThisStr()); }
        OovString getName() const       // Without extension
            { return FilePathGetFileName(getThisStr()); }
        OovString getNameExt() const
            { return FilePathGetFileNameExt(getThisStr()); }
        OovString getExtension() const
            { return FilePathGetFileExt(getThisStr()); }
        OovString getWithoutEndPathSep() const
            { return FilePathGetWithoutEndPathSep(getThisStr()); }
#ifndef __linux__
        OovString getAsWindowsPath() const
            { return FilePathGetAsWindowsPath(getThisStr()); }
#endif
        bool hasExtension() const
            { return FilePathHasExtension(getThisStr()); }
        bool matchExtension(OovStringRef const path) const
            { return FilePathMatchExtension(getThisStr(), path); }
        bool isDirOnDisk(OovStatus &status) const
            { return FileIsDirOnDisk(getThisStr(), status); }
        bool isFileOnDisk(OovStatus &status) const
            { return FileIsFileOnDisk(getThisStr(), status); }
        OovStatusReturn deleteFile() const
            { return FileDelete(getThisStr()); }
        OovStatusReturn ensurePathExists()
            { return FileEnsurePathExists(getThisStr()); }

    private:
        char const * getThisStr() const
            { return static_cast<T_Str const *>(this)->getStr(); }
    };

/// Provides functions for accessing an immutable path.
class FilePathRef:public FilePathRefInterface<FilePathRef>, OovStringRef
    {
    };

//// In this class, any path that ends with a path separator indicates it
/// is a directory.
/// The special case is when FilePathRef is initialized with an empty string,
/// then a directory is not initialized to "/".
/// Also the path is normalized so that path separators are '/' on both
/// linux and windows.
/// Invalid paths when inside of a FilePath:
///     ".."    A directory will be indicated as "../"
class FilePath:public FilePathRefInterface<FilePath>, public OovString
    {
    public:
        FilePath()
            {}
        FilePath(OovStringRef const filePath, eFilePathTypes fpt)
            { pathStdStr() = normalizePathType(filePath, fpt); }
//      FilePath(std::string const &filePath, eFilePathTypes fpt)
//          { pathStdStr() = normalizePathType(filePath, fpt); }
/*
        // The default operator= works fine since the normalized path is copied.
        FilePath &operator=(FilePath const &filePath)
            {
            pathStdStr() = normalizePathType(filePath.pathStdStr(),
                filePath.getType());
            return *this;
            }
*/
        void setPath(OovStringRef const path, eFilePathTypes fpt)
            { pathStdStr() = normalizePathType(path, fpt); }
        eFilePathTypes getType() const;

        // Currently stops at dir spec if it exists.
        FilePath getParent() const;
        int comparePaths(OovStringRef const path2) const
            { return FilePathComparePaths(getStr(), path2); }

        /// Erases anything after the position and appends with an intervening
        /// path separator.
        /// @param pathPart The part of the path to append.
        /// @param pos The offset from the start of the 'this' path.
        void appendPathAtPos(OovStringRef const pathPart, size_t pos);

        /// Erases anything after the position and appends with an intervening
        /// path separator, and appends an ending path separator to the result.
        /// @param pathPart The part of the path to append.
        /// @param pos The offset from the start of the 'this' path.
        void appendDirAtPos(OovStringRef const pathPart, size_t pos);

        /// Appends either a directory or file to a path.
        /// @param pathPart The part of the path to append.
        /// @param fpt The type of part to append.
        void appendPart(OovStringRef const pathPart, eFilePathTypes fpt);

        /// Append a part of a path to the end of this path.
        /// @param pathPart The part of the path to append.
        void appendDir(OovStringRef const pathPart);

        /// Append a file to the end of this path.
        /// @param fileName The file name to append.
        void appendFile(OovStringRef const fileName);

        /// Append an extension to the end of this path.  Appends an extension
        /// separator character if needed, so the extension passed in should
        /// not have an extension separator character.
        /// @param ext The extension to append.
        void appendExtension( OovStringRef const ext);

        /// Discard the directory part of the path.
        void discardDirectory();

        /// Discard the filename and extension part of the path.
        void discardFilename();

        /// Discard the extension part of the path.
        void discardExtension();

        /// Discard the characters after the position.
        /// Keeps the end sep so that this is still indicated as a directory.
        /// @param pos The reference position.
        void discardTail(size_t pos);

        /// Discard the characters before the position.
        /// Erases up to after the position (pos + 1).
        /// A position of zero with no drive or path spec does not erase anything.
        /// @param pos The reference position.
        void discardHead(size_t pos);

        /// Discard leading relative directory specifiers.
        int discardLeadingRelSegments();

        /// Discard the head if it matches.
        /// @param pathPart The head to match.
        void discardMatchingHead(OovStringRef const pathPart);

        /// Get the working directory.
        void getWorkingDirectory();

        /// If the path is already an absolute path, this simply ensures
        /// the path is conformant to the file path type.
        /// If the path is not an absolute path, the working dirctory is
        /// used, and the path is appended to build a path that is
        /// conformant to the file path type.
        /// @param path The path to append to the working directory.
        /// @param fpt The file path type.
        void getAbsolutePath(OovStringRef const path, eFilePathTypes fpt);

        /// Converts the path separators to a forward slash separator, and
        /// converts "/./" to a single forward slash.
        static void normalizePathSeps(std::string &path);

        /// Return the path as an std::string
        std::string &pathStdStr()
            { return *this; }

        /// Return the path as an std::string
        const std::string &pathStdStr() const
            { return *this; }

    protected:
        static std::string normalizePathType(OovStringRef const path, eFilePathTypes fpt);
    };


typedef std::vector<FilePath> FilePaths;

bool FilePathAnyExtensionMatch(FilePaths const &paths, OovStringRef const file);

#endif /* FILEPATH_H_ */
