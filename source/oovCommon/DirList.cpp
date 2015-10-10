//
// C++ Implementation: DirList
//
//  \copyright 2013 DCBlaha.  Distributed under the GPL.
//

#include "DirList.h"
#include <sys/types.h>
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
#ifdef __linux__
#include <unistd.h>             // For rmdir
#endif
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>


OovStatusReturn deleteDir(OovStringRef const path)
    {
    OovStatus status(true, SC_File);
    DIR *dp = opendir(path);
    if(dp)
        {
        struct dirent *dirp;
        while(((dirp = readdir(dp)) != nullptr))
            {
            OovString fullName = path;
            FilePathEnsureLastPathSep(fullName);
            fullName += dirp->d_name;
            status = FileDelete(fullName);
            if(!status.ok())
                {
                break;
                }
            }
        closedir(dp);
        }
    else
        {
        status.set(false, SC_File);
        }
    if(status.ok())
        {
        status.set(rmdir(path) == 0, SC_File);
        }
    return status;
    }

OovStatusReturn recursiveDeleteDir(OovStringRef const path)
    {
    OovStatus status(true, SC_File);
    DIR *dp = opendir(path);
    if(dp)
        {
        struct dirent *dirp;
        while(((dirp = readdir(dp)) != nullptr) && status.ok())
            {
            if(!status.ok())
                {
                break;
                }
            FilePath fullName(path, FP_Dir);
            fullName += dirp->d_name;

            if(fullName.isDirOnDisk(status))
//          if(dirp->d_type == DT_DIR)
                {
                if(strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
                    {
                    status = recursiveDeleteDir(fullName);
                    if(!status.ok())
                        {
                        break;
                        }
                    }
                }
            else
                {
                status = fullName.deleteFile();
                }
            if(!status.ok())
                {
                break;
                }
            }
        closedir(dp);
        }
    else
        {
        status.set(false, SC_File);
        }
    if(status.ok())
        {
        status.set(rmdir(path) == 0, SC_File);
        }
    return status;
    }

/*
bool recursivegetDirListMatchFileName(OovStringRef const path, const FilePath &fn,
        std::vector<std::string> &foundFiles)
    {
    bool success = false;
    DIR *dp = opendir(path);
    if(dp)
        {
        success = true;
        struct dirent *dirp;
        while(((dirp = readdir(dp)) != nullptr) && success)
            {
            if ((strcmp(dirp->d_name, ".") != 0) && (strcmp(dirp->d_name, "..") != 0))
                {
                FilePath foundFn(dirp->d_name, FP_File);
                if(fn.getNameExt() == foundFn.getNameExt())
                    {
                    std::string fullName = path;
                    ensureLastPathSep(fullName);
                    fullName += dirp->d_name;
                    foundFiles.push_back(fullName);
                    }
                }
            else
                {
                getDirListMatchFileName(path, fn, foundFiles);
                }
            }
        closedir(dp);
        }
    return success;
    }
*/

OovStatusReturn getDirListMatchExt(OovStringRef const path, const FilePaths &extensions,
        std::vector<std::string> &fn)
    {
    OovStatus status(true, SC_File);
    DIR *dp = opendir(path);
    if(dp)
        {
        struct dirent *dirp;
        while(((dirp = readdir(dp)) != nullptr) && status.ok())
            {
            if(FilePathAnyExtensionMatch(extensions, dirp->d_name))
                {
                FilePath fullName(path, FP_Dir);
                fullName.appendFile(dirp->d_name);
                fn.push_back(fullName);
                }
            }
        closedir(dp);
        }
    else
        {
        status.set(false, SC_File);
        }
    return status;
    }

OovStatusReturn getDirListMatchExt(OovStringRef const path, const FilePath &ext,
        std::vector<std::string> &fn)
    {
    FilePaths extensions;
    extensions.push_back(ext);
    return getDirListMatchExt(path, extensions, fn);
    }

OovStatusReturn getDirListMatchExt(const std::vector<std::string> &paths,
        const FilePaths &extensions, std::vector<std::string> &fn)
    {
    OovStatus status(true, SC_File);
    for(const auto &path : paths)
        {
        status = getDirListMatchExt(path, extensions, fn);
        if(!status.ok())
            {
            break;
            }
        }
    return status;
    }

OovStatusReturn getDirList(OovStringRef const path, eDirListTypes types, std::vector<std::string> &fn)
    {
    OovStatus status(true, SC_File);
    DIR *dp = opendir(path);
    if(dp)
        {
        struct dirent *dirp;
        while((dirp = readdir(dp)) != nullptr)
            {
            FilePath fullName(path, FP_Dir);
            fullName.appendDir(dirp->d_name);
//          if(dirp->d_type == DT_DIR)          // Some OS's don't have this.
            if(FileIsDirOnDisk(fullName, status))
                {
                if(types & DL_Dirs)
                    {
                    if(dirp->d_name[0] != '.')
                        {
                        fn.push_back(fullName);
                        }
                    }
                }
            else
                {
                if(types & DL_Files)
                    {
                    FilePath fileName(path, FP_Dir);
                    fileName.appendFile(dirp->d_name);
                    fn.push_back(fileName);
                    }
                }
            if(!status.ok())
                {
                break;
                }
            }
        closedir(dp);
        }
    else
        {
        status.set(false, SC_File);
        }
    return status;
    }

// The wildcardStr can have an asterisk, but must be at the end of
// a subdirectory. For example: "\Qt*\mingw*\"
bool subDirMatch(OovStringRef const wildCardStr, OovStringRef const pathStr)
    {
    const char *wcStr = wildCardStr;
    const char *pStr = pathStr;
    bool match = true;
    while(*wcStr)
        {
        switch(*wcStr)
            {
            case '*':
                while(*pStr != '/' && *pStr)
                    {
                    pStr++;
                    }
                break;

            default:
                if(toupper(*wcStr) == toupper(*pStr))
                    {
                    pStr++;
                    }
                else
                    {
                    match = false;
                    }
            }
        wcStr++;
        }
    return(match && *wcStr == *pStr);
    }

// Returns matching subdirectories.
std::vector<std::string> matchDirs(FilePaths const &wildCardDirs)
    {
    std::vector<std::string> subDirs;
    OovStatus status(true, SC_File);
    for(auto const &wcStr : wildCardDirs)
        {
        status = getDirList(wcStr, DL_Dirs, subDirs);
        if(!status.ok())
            {
            OovString err = "Unable to read dir ";
            err += wcStr;
            status.report(ET_Error, err);
            break;
            }
        }
    return subDirs;
    }

OovString const findMatchingDir(FilePaths const &startingDirs, OovStringRef const wildCardStr)
    {
    OovString matchedDir;
    FilePath wildCardPattern(wildCardStr, FP_Dir);
    FilePaths searchDirs = startingDirs;
    size_t pos = wildCardPattern.getPosStartDir();
    while(1)
        {
        std::vector<std::string> foundSubDirs = matchDirs(searchDirs);

        // Filter and copy matching found directories to new search directories.
        searchDirs.clear();
        pos = wildCardPattern.getPosRightPathSep(pos, RP_RetPosNatural);
        for(auto const &foundDir : foundSubDirs)
            {
            FilePaths patterns;
            for(auto const &startDir : startingDirs)
                {
                FilePath pattern(startDir, FP_Dir);
                pattern.appendDir(wildCardPattern.getHead(pos));
                if(subDirMatch(pattern, foundDir))
                    {
                    searchDirs.push_back(FilePath(foundDir, FP_Dir));
                    }
                }
            }
        if(pos == wildCardPattern.numBytes()-1)
            {
            break;
            }
        }
    if(searchDirs.size() > 0)
        {
        matchedDir = searchDirs[0];
        }
    return matchedDir;
    }

dirRecurser::~dirRecurser()
    {}

OovStatusReturn dirRecurser::recurseDirs(OovStringRef const srcDir)
    {
    OovStatus status(true, SC_File);
    DIR *dp = opendir(srcDir);
    if(dp)
        {
        struct dirent *dirp;
        bool success = true;
        while(((dirp = readdir(dp)) != nullptr) && success && status.ok())
            {
            if ((strcmp(dirp->d_name, ".") != 0) && (strcmp(dirp->d_name, "..") != 0))
                {
                FilePath fullName(srcDir, FP_Dir);
                fullName += dirp->d_name;
                if(FileIsDirOnDisk(fullName, status))
                    {
                    status = recurseDirs(fullName);
                    }
                else
                    {
                    success = processFile(fullName);
                    }
                }
            }
        closedir(dp);
        }
    else
        {
        status.set(false, SC_File);

        }
    return status;
    }

