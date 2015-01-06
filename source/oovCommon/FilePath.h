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


// non-modifying path functions
size_t rfindPathSep(OovStringRef const path, size_t startPos = std::string::npos);
size_t findPathSep(OovStringRef const path, size_t startPos = std::string::npos);

// modifying path functions
void ensureLastPathSep(std::string &path);
void removePathSep(std::string &path, int pos);
void quoteCommandLinePath(std::string &libfilePath);

OovString makeExeFilename(OovStringRef const rootFn);
// For some reason, oovEdit requires this?
std::string fixFilePath(OovStringRef const fullFn);

// File operations
bool ensurePathExists(OovStringRef const path);
bool fileExists(OovStringRef const path);
void deleteFile(OovStringRef const path);
void renameFile(OovStringRef const oldPath, OovStringRef const newPath);
bool getFileTime(OovStringRef const path, time_t &time);


/// Provides functions for accessing an immutable path.
class FilePathImmutable
    {
    public:
    FilePathImmutable():
	    mPos(0)
	    {}
	size_t moveToStartDir(OovStringRef const path);
	size_t moveToEndDir(OovStringRef const path);
	size_t moveToExtension(OovStringRef const path);
	// Currently stops at dir spec if it exists.
	size_t moveLeftPathSep(OovStringRef const path);
	size_t moveRightPathSep(OovStringRef const path);
	OovString getHead(OovStringRef const path) const;
	OovString getTail(OovStringRef const path) const;
	OovString getPathSegment(OovStringRef const path) const;
	size_t findPathSegment(OovStringRef const path,
		OovStringRef const seg) const;
	OovString getDrivePath(OovStringRef const path) const;
	OovString getName(OovStringRef const path) const;	// Without extension
	OovString getNameExt(OovStringRef const path) const;
	OovString getExtension(OovStringRef const path) const;
	OovString getWithoutEndPathSep(OovStringRef const path) const;
	bool matchExtension(OovStringRef const path1, OovStringRef const path2) const;

	static bool isAbsolutePath(OovStringRef const path);
	static bool isPathSep(OovStringRef const path, int pos)
	    { return(path[pos] == '/' || path[pos] == '\\'); }
	static bool isEndPathSep(OovStringRef const path);
	static bool isExtensionSep(OovStringRef const path, int pos)
	    { return(path[pos] == '.'); }
	static size_t findExtension(OovStringRef const path);
	static bool hasExtension(OovStringRef const path)
	    { return(findExtension(path) != std::string::npos); }
	static bool isDirOnDisk(OovStringRef const path);
	static int comparePaths(OovStringRef const path1, OovStringRef const path2);

    protected:
	size_t getPos() const
	    { return mPos; }

    private:
	size_t mPos;
    };

enum eFilePathTypes { FP_File, FP_Dir, FP_Ext };

/// In this class, any path that ends with a path separator is a directory.
/// The special case is when FilePathRef is initialized with an empty string,
/// then a directory is not initialized to "/".
class FilePath:public OovString, public FilePathImmutable
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

	size_t moveToStartDir()
	    { return FilePathImmutable::moveToStartDir(c_str()); }
	size_t moveToEndDir()
	    { return FilePathImmutable::moveToEndDir(c_str()); }
	size_t moveToExtension()
	    { return FilePathImmutable::moveToExtension(c_str()); }
	// Currently stops at dir spec if it exists.
	size_t moveLeftPathSep()
	    { return FilePathImmutable::moveLeftPathSep(c_str()); }
	size_t moveRightPathSep()
	    { return FilePathImmutable::moveRightPathSep(c_str()); }
	OovString getHead() const
	    { return FilePathImmutable::getHead(c_str()); }
	OovString getTail() const
	    { return FilePathImmutable::getTail(c_str()); }
	OovString getPathSegment() const
	    { return FilePathImmutable::getPathSegment(c_str()); }
	size_t findPathSegment(OovStringRef const seg) const
	    { return FilePathImmutable::findPathSegment(c_str(), seg); }
	OovString getDrivePath() const
	    { return FilePathImmutable::getDrivePath(c_str()); }
	OovString getName() const	// Without extension
	    { return FilePathImmutable::getName(c_str()); }
	OovString getNameExt() const
	    { return FilePathImmutable::getNameExt(c_str()); }
	OovString getExtension() const
	    { return FilePathImmutable::getExtension(c_str()); }
        FilePath getParent() const;
        OovString getWithoutEndPathSep() const
	    { return FilePathImmutable::getWithoutEndPathSep(c_str()); }
	bool matchExtension(OovStringRef const path2) const
	    { return FilePathImmutable::matchExtension(c_str(), path2); }
	int comparePaths(OovStringRef const path2) const
	    { return FilePathImmutable::comparePaths(c_str(), path2); }

	void appendPathAtPos(OovStringRef const pathPart);
	void appendDirAtPos(OovStringRef const pathPart);
	void appendPart(OovStringRef const pathPart, eFilePathTypes fpt);
	void appendDir(OovStringRef const pathPart);
	void appendFile(OovStringRef const fileName);
	void appendExtension( OovStringRef const fileName);
	void discardDirectory();
	void discardFilename();
	void discardExtension();
	// Keeps the end sep so that this is still indicated as a directory.
	void discardTail();
	void discardHead();
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

bool anyExtensionMatch(FilePaths const &paths, OovStringRef const file);

#endif /* FILEPATH_H_ */
