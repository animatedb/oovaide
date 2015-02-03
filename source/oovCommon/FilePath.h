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
//	The parts are drive, directory segments..., filename, extension.
// Example: "c:/dirseg1/dirseg2/name.ext"
// A drive spec is considered to be anything up to the last colon.

// See FilePath for rules on the paths when inside of a FilePath class.

enum eReturnPosition {
    RP_RetPosNatural,	// Return the most natural position in the path.
    RP_RetPosFailure	// Return std::string::npos if the position is not in the path.
};

//***** non-modifying path functions
// Returns the position after the drive spec. If no drive spec, returns the
// beginning (0).
size_t FilePathGetPosStartDir(OovStringRef const path);
// Returns the position of the last path separator. If no path separators,
// returns the start dir pos.
size_t FilePathGetPosEndDir(OovStringRef const path);
// Returns the position of the extension separator. Since a period may be
// used for relative path specs, a check is made to see if the period is
// after a path separator, and only then is it considered an extension.
// If the extension is not present, the last character position is returned.
size_t FilePathGetPosExtension(OovStringRef const path, eReturnPosition rp);
// Only case insensitive comparisons are done on Windows.
// Returns std::string::npos if the segment is not found.
/// @todo - the following should probably be changed.
// The position returned is pointing to the segment. (If segment starts
// with a path separator, the position will contains the path separator)
size_t FilePathGetPosSegment(OovStringRef const path, OovStringRef const seg);
// If there is no left path separator, the beginning (0) is returned.
size_t FilePathGetPosLeftPathSep(OovStringRef const path, size_t pos, eReturnPosition rp);
size_t FilePathGetPosRightPathSep(OovStringRef const path, size_t pos, eReturnPosition rp);

// If the first character is a path separator or if a drive spec is present
// with a path separator after it, then the path is considered to be absolute.
bool FilePathIsAbsolutePath(OovStringRef const path);
bool FilePathIsPathSep(OovStringRef const path, size_t pos);
// Returns true if the path ends with a path separator.
bool FilePathIsEndPathSep(OovStringRef const path);
bool FilePathIsExtensionSep(OovStringRef const path, size_t pos);
bool FilePathHasExtension(OovStringRef const path);
bool FilePathMatchExtension(OovStringRef const path1, OovStringRef const path2);
int FilePathComparePaths(OovStringRef const path1, OovStringRef const path2);

OovString FilePathGetHead(OovStringRef const path, size_t pos);
OovString FilePathGetTail(OovStringRef const path, size_t pos);
// The segment is returned without path separators.
// The position can be pointing to the anywhere within the desired segment.
// If it is pointing to a path separator, the segment following the path
// separator is returned.
// If the segment is not found, this returns std::string::npos.
OovString FilePathGetSegment(OovStringRef const path, size_t pos);
OovString FilePathGetDrivePath(OovStringRef const path);
OovString FilePathGetFileName(OovStringRef const path);
OovString FilePathGetFileNameExt(OovStringRef const path);
OovString FilePathGetFileExt(OovStringRef const path);
OovString FilePathGetWithoutEndPathSep(OovStringRef const path);

//***** modifying path functions
void FilePathEnsureLastPathSep(std::string &path);
void FilePathRemovePathSep(std::string &path, int pos);
void FilePathQuoteCommandLinePath(std::string &libfilePath);
OovString FilePathMakeExeFilename(OovStringRef const rootFn);
// For some reason, oovEdit requires this?
std::string FilePathFixFilePath(OovStringRef const fullFn);

//***** File/disk operations
bool FileEnsurePathExists(OovStringRef const path);
bool FileIsFileOnDisk(OovStringRef const path);
bool FileIsDirOnDisk(OovStringRef const path);
bool FileGetFileTime(OovStringRef const path, time_t &time);
void FileDelete(OovStringRef const path);
void FileRename(OovStringRef const oldPath, OovStringRef const newPath);

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
	OovString getName() const	// Without extension
	    { return FilePathGetFileName(getThisStr()); }
	OovString getNameExt() const
	    { return FilePathGetFileNameExt(getThisStr()); }
	OovString getExtension() const
	    { return FilePathGetFileExt(getThisStr()); }
	OovString getWithoutEndPathSep() const
	    { return FilePathGetWithoutEndPathSep(getThisStr()); }

	bool matchExtension(OovStringRef const path) const
	    { return FilePathMatchExtension(getThisStr(), path); }
	bool isDirOnDisk() const
	    { return FileIsDirOnDisk(getThisStr()); }
	bool isFileOnDisk() const
	    { return FileIsFileOnDisk(getThisStr()); }
	void deleteFile() const
	    { FileDelete(getThisStr()); }
	void ensurePathExists()
	    { FileEnsurePathExists(getThisStr()); }

    private:
	char const * const getThisStr() const
	    { return static_cast<T_Str const *>(this)->getStr(); }
    };

/// Provides functions for accessing an immutable path.
class FilePathRef:public FilePathRefInterface<FilePathRef>, OovStringRef
    {
    };

enum eFilePathTypes { FP_File, FP_Dir, FP_Ext };

//// In this class, any path that ends with a path separator indicates it
/// is a directory.
/// The special case is when FilePathRef is initialized with an empty string,
/// then a directory is not initialized to "/".
/// Also the path is normalized so that path separators are '/' on both
/// linux and windows.
/// Invalid paths when inside of a FilePath:
///	".."	A directory will be indicated as "../"
class FilePath:public FilePathRefInterface<FilePath>, public OovString
    {
    public:
	FilePath()
	    {}
	FilePath(OovStringRef const filePath, eFilePathTypes fpt)
	    { pathStdStr() = normalizePathType(filePath, fpt); }
//	FilePath(std::string const &filePath, eFilePathTypes fpt)
//	    { pathStdStr() = normalizePathType(filePath, fpt); }
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

	void appendPathAtPos(OovStringRef const pathPart, size_t pos);
	void appendDirAtPos(OovStringRef const pathPart, size_t pos);
	void appendPart(OovStringRef const pathPart, eFilePathTypes fpt);
	void appendDir(OovStringRef const pathPart);
	void appendFile(OovStringRef const fileName);
	void appendExtension( OovStringRef const fileName);
	void discardDirectory();
	void discardFilename();
	void discardExtension();
	// Keeps the end sep so that this is still indicated as a directory.
	void discardTail(size_t pos);
	// Erases up to after the position (pos + 1).
	void discardHead(size_t pos);
	int discardLeadingRelSegments();
	void discardMatchingHead(OovStringRef const pathPart);
	void getWorkingDirectory();
	void getAbsolutePath(OovStringRef const path, eFilePathTypes fpt);
	static void normalizePathSeps(std::string &path);

	std::string &pathStdStr()
	    { return *this; }
	const std::string &pathStdStr() const
	    { return *this; }

    protected:
	static std::string normalizePathType(OovStringRef const path, eFilePathTypes fpt);
    };


typedef std::vector<FilePath> FilePaths;

bool FilePathAnyExtensionMatch(FilePaths const &paths, OovStringRef const file);

#endif /* FILEPATH_H_ */
