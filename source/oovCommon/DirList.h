//
// C++ Interface: DirList
//
//  \copyright 2013 DCBlaha.  Distributed under the GPL.

#ifndef DirList_h
#define DirList_h

#include <string>
#include <vector>
#include "FilePath.h"

void deleteDir(char const * const path);
void recursiveDeleteDir(char const * const path);
bool getDirListMatchExt(char const * const path, const FilePaths &extensions,
	std::vector<std::string> &fn);
bool getDirListMatchExt(char const * const path, const FilePath &ext,
	std::vector<std::string> &fn);
bool getDirListMatchExt(const std::vector<std::string> &paths,
	const FilePaths &extensions, std::vector<std::string> &fn);
bool getDirListMatchExt(const std::vector<std::string> &paths,
	const FilePaths &extensions, std::vector<std::string> &fn);
enum eDirListTypes { DL_Files, DL_Dirs };
bool getDirList(char const * const path, eDirListTypes, std::vector<std::string> &fn);

/// Recursivley walks a directory, and calls the processFile
/// function as each file is found.
class dirRecurser
{
public:
    virtual ~dirRecurser()
	{}
    bool recurseDirs(char const * const path);
    virtual bool processFile(const std::string &filePath) = 0;
};

#endif
