/*
 * FilePath.cpp
 *
 *  Created on: Oct 14, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "FilePath.h"
#include "OovString.h"
#include <string.h>
#include "File.h"       // For sleepMs at the moment.
#include "OovError.h"
#ifdef __linux__
#include <unistd.h>
#else
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
#include <direct.h>
#endif

#define DEBUG_PATHS 0
#if(DEBUG_PATHS)
#define CHECKSIZE(file, line, strSize, pos) checkSize(file, line, strSize, pos)
static void checkSize(OovStringRef const file, int line, size_t strSize, size_t pos)
    {
    if(pos > strSize)
        {
        assert(false);
        LogAssertFile(file, line, "size");
        }
    }
#else
#define CHECKSIZE(file, line, strSize, pos)
#endif

// returns std::string::npos if not found.
static size_t findPathSep(OovStringRef const path, size_t startPos = 0)
    {
    const OovString str = path;
    size_t bpos = str.find('\\', startPos);
    size_t fpos = str.find('/', startPos);
    size_t pos = std::string::npos;
    if(bpos != std::string::npos && fpos != std::string::npos)
        {
        pos = (bpos > fpos) ? fpos : bpos;
        }
    else if(bpos != std::string::npos)
        pos = bpos;
    else if(fpos != std::string::npos)
        pos = fpos;
    return pos;
    }

// returns std::string::npos if not found.
static size_t rfindPathSep(OovStringRef const path, size_t startPos = std::string::npos)
    {
    const OovString str = path;
    size_t bpos = str.rfind('\\', startPos);
    size_t fpos = str.rfind('/', startPos);
    size_t pos = std::string::npos;
    if(bpos != std::string::npos && fpos != std::string::npos)
        {
        pos = (bpos > fpos) ? bpos : fpos;
        }
    else if(bpos != std::string::npos)
        pos = bpos;
    else if(fpos != std::string::npos)
        pos = fpos;
    return pos;
    }


///////////////


size_t FilePathGetPosStartDir(OovStringRef const path)
    {
    size_t pos = std::string(path.getStr()).rfind(':');
    if(pos == std::string::npos)
        {
        pos = 0;
        }
    else
        {
        pos++;
        }
    return pos;
    }

size_t FilePathGetPosEndDir(OovStringRef const path)
    {
    size_t pos;
    if(FilePathIsEndPathSep(path))
        {
        pos = path.numBytes() - 1;
        }
    else
        {
        pos = rfindPathSep(path);
        if(pos == std::string::npos)
            pos = FilePathGetPosStartDir(path);
        }
    return pos;
    }

size_t FilePathGetPosExtension(OovStringRef const path, eReturnPosition rp)
    {
    size_t pathPos = rfindPathSep(path);
    size_t extPos = std::string(path).rfind('.');
    if((pathPos != std::string::npos) && (extPos != std::string::npos) && (extPos < pathPos))
        {
        if(rp == RP_RetPosNatural)
            extPos = std::string(path).length();
        else
            extPos = std::string::npos;
        }
    return extPos;
    }

size_t FilePathGetPosLeftPathSep(OovStringRef const path, size_t pos, eReturnPosition rp)
    {
    if(FilePathIsPathSep(path, pos) && pos > 0)
        pos--;
    pos = rfindPathSep(path, pos);
    if(pos == std::string::npos)
        {
        if(rp == RP_RetPosNatural)
            {
            pos = FilePathGetPosStartDir(path);
            }
        }
    return pos;
    }

size_t FilePathGetPosRightPathSep(OovStringRef const path, size_t pos, eReturnPosition rp)
    {
    if(FilePathIsPathSep(path, pos) && pos < path.numBytes())
        pos++;
    pos = findPathSep(path, pos);
    if(pos == std::string::npos)
        {
        if(rp == RP_RetPosNatural)
            {
            pos = FilePathGetPosEndDir(path);
            }
        }
    return pos;
    }

size_t FilePathGetPosSegment(OovStringRef const path, OovStringRef const seg)
    {
    OovString lowerPath;
    // Only do case insensitive compares for Windows.
#ifdef __linux__
    lowerPath = path;
#else
    lowerPath.setLowerCase(path);
#endif
    OovString lowerSeg;
#ifdef __linux__
    lowerSeg = seg;
#else
    lowerSeg.setLowerCase(seg);
#endif
    size_t pos = lowerPath.find(lowerSeg);
    size_t foundPos = std::string::npos;
    if(pos != std::string::npos)
        {
        if(pos > 0)
            {
            size_t endPos = pos + lowerSeg.length();
            if(FilePathIsPathSep(path, pos-1) &&
                    FilePathIsPathSep(path, endPos))
                {
                foundPos = pos-1;
                }
            }
        }
    return foundPos;
    }


//////////////


OovString FilePathGetHead(OovStringRef const path, size_t pos)
    {
    return std::string(path).substr(0, pos+1);
    }

OovString FilePathGetTail(OovStringRef const path, size_t pos)
    {
    return std::string(path).substr(pos);
    }

OovString FilePathGetSegment(OovStringRef const path, size_t pos)
    {
    OovString part;
    size_t startPos = 0;
    if(FilePathIsPathSep(path, pos))
        startPos = pos+1;
    else
        {
        startPos = rfindPathSep(path, pos);
        if(startPos != std::string::npos)
            startPos++;
        else
            startPos=0;
        }
    size_t endPos = findPathSep(path, startPos);
    if(endPos != std::string::npos)
        {
        --endPos;
        part = std::string(path).substr(startPos, endPos-startPos+1);
        }
    return part;
    }

OovString FilePathGetWithoutEndPathSep(
        OovStringRef const path)
    {
    OovString str = path;
    FilePathRemovePathSep(str, str.length()-1);
    return str;
    }

OovString FilePathGetDrivePath(OovStringRef const path)
    {
    FilePath fp(path, FP_File);
    return fp.getHead(fp.getPosEndDir());
    }

OovString FilePathGetFileName(OovStringRef const path)
    {
    FilePath fp(path, FP_File);
    size_t pos = fp.getPosEndDir();
    if(pos != 0)
        {
        fp.discardHead(pos);
        }
    fp.discardExtension();
    return fp;
    }

OovString FilePathGetFileNameExt(OovStringRef const path)
    {
    FilePath fp(path, FP_File);
    fp.discardHead(fp.getPosEndDir());
    return fp;
    }

OovString FilePathGetFileExt(OovStringRef const path)
    {
    OovString ext;
    size_t extPos = FilePathGetPosExtension(path, RP_RetPosFailure);
    if(extPos != std::string::npos)
        {
        ext = std::string(path).substr(extPos, strlen(path) - extPos);
        }
    return ext;
    }

#ifndef __linux__
OovString FilePathGetAsWindowsPath(OovStringRef const path)
    {
    OovString windowsPath = path;
    windowsPath.replaceStrs("/", "\\");
    return windowsPath;
    }
#endif

//////////////


void FilePathEnsureLastPathSep(std::string &path)
    {
    if(!FilePathIsEndPathSep(path))
        path += '/';
    }

bool FileEnsurePathExists(OovStringRef const path)
    {
    bool success = true;

    // Walk up the tree to find a base that exists.
    FilePath fp(path, FP_File);
    size_t pos = fp.getPosEndDir();
    while(pos != 0)
        {
        fp.discardTail(pos);
        if(fp.isDirOnDisk(success))
            break;
        else
            pos = fp.getPosLeftPathSep(pos, RP_RetPosFailure);
        }
    while(pos != std::string::npos && pos != 0 && success)
        {
        pos = findPathSep(path, pos);
        if(pos != std::string::npos)
            {
            OovString partPath = path;
            partPath.resize(pos);
            if(!FileIsDirOnDisk(partPath, success))
                {
#ifdef __linux__
                success = (mkdir(partPath.getStr(), 0x1FF) == 0);       // 0777
#else
                success = (_mkdir(partPath.getStr()) == 0);
#endif
                }
            pos++;
            }
        }
    return success;
    }

void FilePathRemovePathSep(std::string &path, size_t pos)
    {
    CHECKSIZE(__FILE__, __LINE__, path.size(), pos);
    if(path[pos] == '/' || path[pos] == '\\' )
        {
        CHECKSIZE(__FILE__, __LINE__, path.size(), pos);
        path.erase(path.begin() + pos);
        }
    }

void FilePathQuoteCommandLinePath(std::string &str)
    {
// In Windows, something like "-I\te st\" must be quoted as "-I\te st\\".
// So for now, this function does not quote all arguments correctly.

//#ifndef __linux__
    if(str.length() > 0)
        {
        if(str[0] != '\"')
            {
/*
            for(size_t i=0; i<str.length(); i++)
                {
printf("%d %c\n", i, str[i]);
                if(str[i] == '\\')
                    {
                    str.insert(i, 1, '\\');
                    i++;
printf(" ADD %d %c\n", i, str[i]);
                    }
                if(str[i] == '\"')
                    {
                    str.insert(i, 1, '\\');
                    i++;
                    }
                }
*/
            if(str.find(' ') != std::string::npos)
                {
                str.insert(0, 1, '\"');
                str.append(1, '\"');
                }
            }
        }
//#endif
    }

#ifdef __linux__
OovString FilePathMakeExeFilename(OovStringRef const  rootFn)
    { return rootFn.getStr(); }
#else
OovString FilePathMakeExeFilename(OovStringRef const  rootFn)
    { return (std::string(rootFn) + ".exe"); }
#endif

// Under Windows, for some reason there are doubled slashes in some cases.
#ifdef __linux__
std::string FilePathFixFilePath(OovStringRef const fullFn)
    {
    return fullFn.getStr();
    }
#else
std::string FilePathFixFilePath(OovStringRef const fullFn)
    {
    std::string temp;
    for(size_t i=0; i<strlen(fullFn); i++)
            {
            if(fullFn[i] != '\\' || fullFn[i+1] != '\\')
                temp += fullFn[i];
            }
    return temp;
    }
#endif


bool FilePathMatchExtension(OovStringRef const path1, OovStringRef const path2)
    {
    std::string ext1 = FilePathGetFileExt(path1);
    std::string ext2 = FilePathGetFileExt(path2);
    return(ext1.length() > 0 && ext1.compare(ext2) == 0);
    }

bool FilePathIsEndPathSep(OovStringRef const path)
    {
    int len = std::string(path).length();
    return(FilePathIsPathSep(path, len-1));
    }

bool FilePathIsAbsolutePath(OovStringRef const path)
    {
    bool absDrv = false;
    size_t drvPos = std::string(path).find(':');
    if(drvPos != std::string::npos)
        {
        absDrv = FilePathIsPathSep(path, drvPos+1);
        }
    return(FilePathIsPathSep(path, 0) || absDrv);
    }

bool FilePathIsPathSep(OovStringRef const path, size_t pos)
    {
    return(pos != std::string::npos && (path[pos] == '/' || path[pos] == '\\'));
    }

bool FilePathIsExtensionSep(OovStringRef const path, size_t pos)
    {
    return(pos != std::string::npos && path[pos] == '.');
    }

bool FilePathHasExtension(OovStringRef const path)
    {
    return(FilePathGetPosExtension(path, RP_RetPosFailure) != std::string::npos);
    }

int FilePathComparePaths(OovStringRef const path1,
        OovStringRef const path2)
    {
#ifdef __linux__
    return strcmp(path1, path2);
#else
    return StringCompareNoCase(path1, path2);
#endif
    }

bool FilePathAnyExtensionMatch(FilePaths const &paths, OovStringRef const file)
    {
    bool match = false;
    for(auto const &path : paths)
        {
        if(path.matchExtension(file))
            {
            match = true;
            break;
            }
        }
    return match;
    }


//////////////

bool FileIsFileOnDisk(OovStringRef const path, bool &success)
    {
    OovString tempPath = path;
    FilePathRemovePathSep(tempPath, tempPath.size()-1);
    struct OovStat32 statval;
    bool statRet = (OovStat32(tempPath.getStr(), &statval) == 0);
    success = statRet;
    if(!statRet && errno == ENOENT)
        success = true;
    return(statRet && !S_ISDIR(statval.st_mode));
    }

bool FileIsDirOnDisk(OovStringRef const path, bool &success)
    {
    struct OovStat32 statval;
    bool statRet = (OovStat32(path, &statval) == 0);
    success = statRet;
    if(!statRet && errno == ENOENT)
        success = true;
    return(success && S_ISDIR(statval.st_mode));
    }

void FileDelete(OovStringRef const path)
    {
    unlink(path.getStr());
    }

void FileWaitForDirDeleted(OovStringRef const path, int waitMs)
    {
#ifndef __linux__
    int minWait = 100;
    int count = waitMs / minWait;
    bool deleted = false;
    bool success = true;
    for(int tries=0; tries<count && success; tries++)
        {
        if(!FileIsDirOnDisk(path, success))
            {
            deleted = true;
            break;
            }
        else
            {
            sleepMs(minWait);
            }
        }
    if(!success || !deleted)
        {
        OovString str = "Unable to delete directory: ";
        str += path;
        OovError::report(ET_Error, str);
        }
#endif
    }

void FileRename(OovStringRef const  oldPath, OovStringRef const  newPath)
    {
    rename(oldPath.getStr(), newPath.getStr());
    }

bool FileGetFileTime(OovStringRef const path, time_t &time)
    {
    struct OovStat32 srcFileStat;
    bool success = (OovStat32(path.getStr(), &srcFileStat) == 0);
    if(success)
        time = srcFileStat.st_mtime;
    return success;
    }

///////////

bool FileStat::isOutputOld(OovStringRef const outputFn,
        OovStringRef const inputFn)
    {
    time_t outTime;
    time_t inTime;
    bool success = FileGetFileTime(outputFn, outTime);
    bool old = !success;
    if(success)
        {
        success = FileGetFileTime(inputFn, inTime);
        if(success)
            old = inTime > outTime;
        else
            old = true;
        }
    return old;
    }

bool FileStat::isOutputOld(OovStringRef const outputFn,
        OovStringVec const &inputs, size_t *oldIndex)
    {
    bool old = false;
    for(size_t i=0; i<inputs.size(); i++)
        {
        if(isOutputOld(outputFn, inputs[i]))
            {
            old = true;
            if(oldIndex)
                *oldIndex = i;
            break;
            }
        }
    return old;
    }


///////////

std::string FilePath::normalizePathType(OovStringRef const path, eFilePathTypes fpt)
    {
    std::string dst;
    if(path)
        {
        if(fpt == FP_Dir)
            {
            dst = path;
            if(dst.length() != 0)
                {
                if(!FilePathIsEndPathSep(path))
                    dst.append("/");
                }
            }
        else if(fpt == FP_File)
            {
            dst = path;
            }
        else if(fpt == FP_Ext)
            {
            if(!FilePathIsExtensionSep(path, 0))
                dst.append(".");
            dst.append(path);
            }
        }
    normalizePathSeps(dst);
    return dst;
    }

eFilePathTypes FilePath::getType() const
    {
    eFilePathTypes fpt;
    if(pathStdStr()[0] == '.')
        fpt = FP_Ext;
    else if(FilePathIsEndPathSep(pathStdStr()))
        fpt = FP_Dir;
    else
        fpt = FP_File;
    return fpt;
    }

void FilePath::appendPathAtPos(OovStringRef const pathPart, size_t pos)
    {
    char const *pp = pathPart;
    if(pos != std::string::npos)
        {
        if(FilePathIsPathSep(pp, 0))
            pp++;
        if(FilePathIsPathSep(pathStdStr(), pos))
            {
            CHECKSIZE(__FILE__, __LINE__, size(), pos+1);
            pathStdStr().erase(pos+1);
            }
        else
            {
            CHECKSIZE(__FILE__, __LINE__, size(), pos);
            pathStdStr().erase(pos);
            }
        }
    pathStdStr().append(pp);
    }

void FilePath::appendDirAtPos(OovStringRef const pathPart, size_t pos)
    {
    appendPathAtPos(pathPart, pos);
    if(!FilePathIsEndPathSep(pathStdStr()))
        pathStdStr().append("/");
    }

void FilePath::appendPart(OovStringRef const pathPart, eFilePathTypes fpt)
    {
    if(fpt == FP_Dir)
        appendDir(pathPart);
    else
        appendFile(pathPart);
    }

void FilePath::appendDir(OovStringRef const pathPart)
    {
    appendDirAtPos(pathPart, getPosEndDir());
    }

void FilePath::discardTail(size_t pos)
    {
    // Keep the end sep so that this is still indicated as a directory.
    if(FilePathIsPathSep(pathStdStr(), pos))
        {
        CHECKSIZE(__FILE__, __LINE__, size(), pos+1);
        pathStdStr().erase(pos+1);
        }
    else
        {
        CHECKSIZE(__FILE__, __LINE__, size(), pos);
        pathStdStr().erase(pos);
        }
    }

void FilePath::appendFile(OovStringRef const fileName)
    {
    OovString fn = fileName;
    if(FilePathIsEndPathSep(pathStdStr()))
        {
        FilePathRemovePathSep(fn, 0);
        }
    pathStdStr().append(fn);
    }

void FilePath::appendExtension(OovStringRef const fileName)
    {
    discardExtension();
    /// @todo - ??
    if(FilePathGetPosExtension(fileName, RP_RetPosFailure) == std::string::npos)
        pathStdStr().append(".");
    pathStdStr().append(fileName);
    }

void FilePath::getWorkingDirectory()
    {
    char buf[1000];
    pathStdStr().clear();
    if(getcwd(buf, sizeof(buf)))
        {
        pathStdStr().append(buf);
        }
    FilePathEnsureLastPathSep(pathStdStr());
    }

FilePath FilePath::getParent() const
    {
    FilePath parent(*this);
    size_t pos = parent.getPosLeftPathSep(parent.getPosEndDir(), RP_RetPosNatural);
    parent.discardTail(pos);
    return parent;
    }

void FilePath::discardDirectory()
    {
    discardHead(getPosEndDir());
    }

void FilePath::discardFilename()
    {
    discardTail(getPosEndDir());
    }

void FilePath::discardExtension()
    {
    size_t pos = getPosExtension(RP_RetPosFailure);
    if(pos != std::string::npos)
        discardTail(pos);
    }

void FilePath::discardHead(size_t pos)
    {
    CHECKSIZE(__FILE__, __LINE__, size(), pos+1);
    pathStdStr().erase(0, pos+1);
    }

int FilePath::discardLeadingRelSegments()
    {
    int count = 0;
    bool didSeg = true;
    do
        {
        OovString seg = getPathSegment(0);
        if(seg.compare("..") == 0)
            {
            CHECKSIZE(__FILE__, __LINE__, size(), 3);
            pathStdStr().erase(0, 3);
            count++;
            }
        else if(seg.compare(".") == 0)
            {
            CHECKSIZE(__FILE__, __LINE__, size(), 2);
            pathStdStr().erase(0, 2);
            }
        else
            {
            didSeg = false;
            }
        } while(didSeg);
    return count;
    }

void FilePath::discardMatchingHead(OovStringRef const pathPart)
    {
    std::string part(pathPart);
    if(pathStdStr().compare(0, part.length(), part) == 0)
        {
        CHECKSIZE(__FILE__, __LINE__, size(), part.length());
        pathStdStr().erase(0, part.length());
        }
    }

void FilePath::getAbsolutePath(OovStringRef const path, eFilePathTypes fpt)
    {
    if(FilePathIsAbsolutePath(path))
        {
        if(fpt == FP_Dir)
            appendDir(path);
        else
            appendFile(path);
        }
    else
        {
        getWorkingDirectory();
        FilePath newPath(path, fpt);
        int count = newPath.discardLeadingRelSegments();
        size_t pos = getPosEndDir();
        for(int i=0; i<count; i++)
            {
            pos = getPosLeftPathSep(pos, RP_RetPosNatural);
            }
        appendPathAtPos(newPath, pos);
        }
    normalizePathSeps(pathStdStr());
    if(fpt == FP_Dir)
        {
        if(!FilePathIsEndPathSep(pathStdStr()))
            pathStdStr().append("/");
        }
    }

void FilePath::normalizePathSeps(std::string &path)
    {
    for(auto &c : path)
        {
        if(c == '\\')
            {
            c = '/';
            }
        }
    while(1)
        {
        size_t pos = path.find("/./");
        if(pos != std::string::npos)
            path.replace(pos, 3, "/");
        else
            break;
        }
    }
