//
// C++ Interface: DirList
//
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#ifndef DirList_h
#define DirList_h

#include <string>
#include <vector>
#include "FilePath.h"

void deleteDir(OovStringRef const path);
void recursiveDeleteDir(OovStringRef const path);
bool getDirListMatchExt(OovStringRef const path, const FilePaths &extensions,
	std::vector<std::string> &fn);
bool getDirListMatchExt(OovStringRef const path, const FilePath &ext,
	std::vector<std::string> &fn);
bool getDirListMatchExt(const std::vector<std::string> &paths,
	const FilePaths &extensions, std::vector<std::string> &fn);
bool getDirListMatchExt(const std::vector<std::string> &paths,
	const FilePaths &extensions, std::vector<std::string> &fn);
// Returns the first matching directory, or an empty string for no match.
// The wildcardStr can have an asterisk, but must be at the end of
// a subdirectory. For example: "\Qt*\mingw*\"
OovString const findMatchingDir(FilePaths const &startingDirs,
	OovStringRef const wildCardStr);
enum eDirListTypes { DL_Files=0x01, DL_Dirs=0x02, DL_Both=DL_Files|DL_Dirs };
// @param fn A vector of full path names. The names are appended to the vector.
bool getDirList(OovStringRef const path, eDirListTypes, std::vector<std::string> &fn);

/// Recursivley walks a directory, and calls the processFile
/// function as each file is found.
class dirRecurser
{
public:
    virtual ~dirRecurser()
	{}
    bool recurseDirs(OovStringRef const path);
    // Return true while success.
    virtual bool processFile(OovStringRef const filePath) = 0;
};

#endif
