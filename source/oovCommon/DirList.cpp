//
// C++ Implementation: DirList
//
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
//

#include "DirList.h"
#include <sys/types.h>
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>


void deleteDir(char const * const path)
    {
    DIR *dp = opendir(path);
    if(dp)
	{
	struct dirent *dirp;
	while(((dirp = readdir(dp)) != nullptr))
	    {
	    std::string fullName = path;
	    ensureLastPathSep(fullName);
	    fullName += dirp->d_name;
	    deleteFile(fullName.c_str());
	    }
	closedir(dp);
	}
    rmdir(path);
    }

void recursiveDeleteDir(char const * const path)
    {
    DIR *dp = opendir(path);
    if(dp)
	{
	struct dirent *dirp;
	while(((dirp = readdir(dp)) != nullptr))
	    {
	    std::string fullName = path;
	    ensureLastPathSep(fullName);
	    fullName += dirp->d_name;
	    if(dirp->d_type == DT_DIR)
		{
		if(strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
		    recursiveDeleteDir(fullName.c_str());
		}
	    else
		deleteFile(fullName.c_str());
	    }
	closedir(dp);
	}
    rmdir(path);
    }

bool getDirListMatchExt(char const * const path, const FilePaths &extensions,
	std::vector<std::string> &fn)
    {
    bool success = false;
    DIR *dp = opendir(path);
    if(dp)
	{
	success = true;
	struct dirent *dirp;
	while(((dirp = readdir(dp)) != nullptr) && success)
	    {
	    if(anyExtensionMatch(extensions, dirp->d_name))
		{
		std::string fullName = path;
		ensureLastPathSep(fullName);
		fullName += dirp->d_name;
		fn.push_back(fullName);
		}
	    }
	closedir(dp);
	}
    return success;
    }

bool getDirListMatchExt(char const * const path, const FilePath &ext,
	std::vector<std::string> &fn)
    {
    FilePaths extensions;
    extensions.push_back(ext);
    return getDirListMatchExt(path, extensions, fn);
    }

bool getDirListMatchExt(const std::vector<std::string> &paths,
	const FilePaths &extensions, std::vector<std::string> &fn)
    {
    bool success = true;
    for(const auto &path : paths)
	{
	success = getDirListMatchExt(path.c_str(), extensions, fn);
	if(!success)
	    break;
	}
    return success;
    }

bool getDirList(char const * const path, eDirListTypes, std::vector<std::string> &fn)
    {
    bool success = false;
    DIR *dp = opendir(path);
    if(dp)
	{
	success = true;
	struct dirent *dirp;
	while(((dirp = readdir(dp)) != nullptr) && success)
	    {
	    FilePath fullName(path, FP_Dir);
	    if(dirp->d_type == DT_DIR)
		fullName.appendDir(dirp->d_name);
	    else
		fullName.appendFile(dirp->d_name);
	    fn.push_back(fullName);
	    }
	closedir(dp);
	}
    return success;
    }

bool dirRecurser::recurseDirs(char const * const srcDir)
    {
    bool success = true;
    DIR *dp = opendir(srcDir);
    if(dp)
	{
	struct dirent *dirp;
	while(((dirp = readdir(dp)) != nullptr) && success)
	    {
	    if ((strcmp(dirp->d_name, ".") != 0) && (strcmp(dirp->d_name, "..") != 0))
		{
		std::string fullName = srcDir;
		ensureLastPathSep(fullName);
		fullName += dirp->d_name;
		struct OovStat32 statval;
		bool success = (OovStat32(fullName.c_str(), &statval) == 0);
		if(success && S_ISREG(statval.st_mode))
		    success = processFile(fullName.c_str());
		else
		    recurseDirs(fullName.c_str());
		}
	    }
	closedir(dp);
	}
    return success;
    }
