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

#include <string>
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
size_t rfindPathSep(char const * const path, size_t startPos = std::string::npos);
size_t findPathSep(char const * const path, size_t startPos = std::string::npos);

// modifying path functions
void ensureLastPathSep(std::string &path);
void removePathSep(std::string &path, int pos);
void quoteCommandLinePath(std::string &libfilePath);

std::string makeExeFilename(char const * const rootFn);
// For some reason, oovEdit requires this?
std::string fixFilePath(char const *fullFn);

// File operations
bool ensurePathExists(char const * const path);
bool fileExists(char const * const path);
void deleteFile(char const * const path);
void renameFile(char const * const oldPath, char const * const newPath);
bool getFileTime(char const * const path, time_t &time);


/// Provides functions for accessing an immutable path.
class FilePathImmutable
    {
    public:
    FilePathImmutable():
	    mPos(0)
	    {}
	size_t moveToStartDir(char const * const path);
	size_t moveToEndDir(char const * const path);
	size_t moveToExtension(char const * const path);
	// Currently stops at dir spec if it exists.
	size_t moveLeftPathSep(char const * const path);
	std::string getHead(char const * const path) const;
	std::string getTail(char const * const path) const;
	std::string getPathSegment(char const * const path) const;
	size_t findPathSegment(char const * const path,
		char const * const seg) const;
	std::string getDrivePath(char const * const path) const;
	std::string getName(char const * const path) const;	// Without extension
	std::string getNameExt(char const * const path) const;
	std::string getExtension(char const * const path) const;
	std::string getWithoutEndPathSep(char const * const path) const;
	bool matchExtension(char const * const path1, char const * const path2) const;

	static bool isAbsolutePath(char const * const path);
	static bool isPathSep(char const * const path, int pos)
	    { return(path[pos] == '/' || path[pos] == '\\'); }
	static bool isEndPathSep(char const * const path);
	static bool isExtensionSep(char const * const path, int pos)
	    { return(path[pos] == '.'); }
	static size_t findExtension(char const * const path);
	static bool hasExtension(char const * const path)
	    { return(findExtension(path) != std::string::npos); }
	static bool isDirOnDisk(char const * const path);
	static int comparePaths(char const * const path1, char const * const path2);

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
class FilePath:public std::string, public FilePathImmutable
    {
    public:
	FilePath()
	    {}
	FilePath(char const * const filePath, eFilePathTypes fpt)
	    { pathStdStr() = normalizePathType(filePath, fpt); }
	FilePath(std::string const &filePath, eFilePathTypes fpt)
	    { pathStdStr() = normalizePathType(filePath.c_str(), fpt); }
/*
        // The default operator= works fine since the normalized path is copied.
        FilePath &operator=(FilePath const &filePath)
            {
            pathStdStr() = normalizePathType(filePath.pathStdStr(),
                filePath.getType());
            return *this;
            }
*/
	void setPath(char const * const path, eFilePathTypes fpt)
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
	std::string getHead() const
	    { return FilePathImmutable::getHead(c_str()); }
	std::string getTail() const
	    { return FilePathImmutable::getTail(c_str()); }
	std::string getPathSegment() const
	    { return FilePathImmutable::getPathSegment(c_str()); }
	size_t findPathSegment(char const * const seg) const
	    { return FilePathImmutable::findPathSegment(c_str(), seg); }
	std::string getDrivePath() const
	    { return FilePathImmutable::getDrivePath(c_str()); }
	std::string getName() const	// Without extension
	    { return FilePathImmutable::getName(c_str()); }
	std::string getNameExt() const
	    { return FilePathImmutable::getNameExt(c_str()); }
	std::string getExtension() const
	    { return FilePathImmutable::getExtension(c_str()); }
        FilePath getParent() const;
	std::string getWithoutEndPathSep() const
	    { return FilePathImmutable::getWithoutEndPathSep(c_str()); }
	bool matchExtension(char const * const path2) const
	    { return FilePathImmutable::matchExtension(c_str(), path2); }
	int comparePaths(char const * const path2) const
	    { return FilePathImmutable::comparePaths(c_str(), path2); }

	void appendPathAtPos(char const * const pathPart);
	void appendDirAtPos(char const * const pathPart);
	void appendPart(char const * const pathPart, eFilePathTypes fpt);
	void appendDir(char const * const pathPart);
	void appendFile(char const * const fileName);
	void appendExtension( char const * const fileName);
	void discardDirectory();
	void discardFilename();
	void discardExtension();
	// Keeps the end sep so that this is still indicated as a directory.
	void discardTail();
	void discardHead();
	int discardLeadingRelSegments();
	void discardMatchingHead(char const * const pathPart);
	void getWorkingDirectory();
	void getAbsolutePath(char const * const path, eFilePathTypes fpt);
	static void normalizePathSeps(std::string &path);

	std::string &pathStdStr()
	    { return *this; }
	const std::string &pathStdStr() const
	    { return *this; }

    protected:
	static std::string normalizePathType(char const * const path, eFilePathTypes fpt);
    };


typedef std::vector<FilePath> FilePaths;

bool anyExtensionMatch(FilePaths const &paths, char const * const file);

#endif /* FILEPATH_H_ */
