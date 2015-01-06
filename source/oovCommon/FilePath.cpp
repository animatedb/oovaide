/*
 * FilePath.cpp
 *
 *  Created on: Oct 14, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "FilePath.h"
#include "OovString.h"
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#else
// Prevent "error: 'off64_t' does not name a type"
#define __NO_MINGW_LFS 1
#include <direct.h>
#endif

void ensureLastPathSep(std::string &path)
    {
    if(!FilePath::isEndPathSep(path))
	path += '/';
    }

bool ensurePathExists(OovStringRef const path)
    {
    bool success = true;

    // Walk up the tree to find a base that exists.
    FilePath fp(path, FP_File);
    size_t pos = fp.moveToEndDir();
    while(pos != std::string::npos && pos != 0)
	{
	fp.discardTail();
	if(fileExists(fp))
	    break;
	else
	    pos = fp.moveLeftPathSep();
	}
    while(pos != std::string::npos && pos != 0 && success)
	{
	pos = findPathSep(path, pos);
	if(pos != std::string::npos)
	    {
	    OovString partPath = path;
	    partPath.resize(pos);
	    if(!fileExists(partPath))
		{
#ifdef __linux__
		success = (mkdir(partPath.getStr(), 0x1FF) == 0);	// 0777
#else
		success = (_mkdir(partPath.getStr()) == 0);
#endif
		}
	    pos++;
	    }
	}
    return success;
    }

void removePathSep(std::string &path, int pos)
    {
    if(path[pos] == '/' || path[pos] == '\\' )
	path.erase(path.begin() + pos);
    }

void quoteCommandLinePath(std::string &str)
    {
    if(str.length() > 0)
	{
	if(str[0] != '\"')
	    {
	    if(str.find(' ') != std::string::npos)
		{
		str.insert(0, 1, '\"');
		str.append(1, '\"');
		}
	    }
	}
    }

size_t findPathSep(OovStringRef const path, size_t startPos)
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

size_t rfindPathSep(OovStringRef const path, size_t startPos)
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

#ifdef __linux__
OovString makeExeFilename(OovStringRef const  rootFn)
    { return rootFn.getStr(); }
#else
OovString makeExeFilename(OovStringRef const  rootFn)
    { return (std::string(rootFn) + ".exe"); }
#endif

// Under Windows, for some reason there are doubled slashes in some cases.
#ifdef __linux__
std::string fixFilePath(OovStringRef const fullFn)
    {
    return fullFn.getStr();
    }
#else
std::string fixFilePath(OovStringRef const fullFn)
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


bool fileExists(OovStringRef const path)
    {
    OovString tempPath = path;
    removePathSep(tempPath, tempPath.size()-1);
    struct OovStat32 statval;
    return(OovStat32(tempPath.getStr(), &statval) == 0);
    }

void deleteFile(OovStringRef const path)
    {
    unlink(path.getStr());
    }

void renameFile(OovStringRef const  oldPath, OovStringRef const  newPath)
    {
    rename(oldPath.getStr(), newPath.getStr());
    }

bool getFileTime(OovStringRef const path, time_t &time)
    {
    struct OovStat32 srcFileStat;
    bool success = (OovStat32(path.getStr(), &srcFileStat) == 0);
    if(success)
	time = srcFileStat.st_mtime;
    return success;
    }

bool anyExtensionMatch(FilePaths const &paths, OovStringRef const file)
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



///////////

size_t FilePathImmutable::moveToStartDir(OovStringRef const path)
    {
    mPos = std::string(path.getStr()).rfind(':');
    if(mPos == std::string::npos)
	mPos = 0;
    return mPos;
    }

size_t FilePathImmutable::moveToEndDir(OovStringRef const path)
    {
    size_t pos;
    if(isEndPathSep(path))
	{
	pos = path.numChars() - 1;
	mPos = pos;
	}
    else
	{
	pos = rfindPathSep(path);
	if(pos == std::string::npos)
	    mPos = path.numChars() - 1;
	else
	    mPos = pos;
	}
    return pos;
    }

size_t FilePathImmutable::moveToExtension(OovStringRef const path)
    {
    size_t pos = findExtension(path);
    if(pos == std::string::npos)
	mPos = 0;
    else
	mPos = pos;
    return pos;
    }

size_t FilePathImmutable::moveLeftPathSep(OovStringRef const path)
    {
    if(isPathSep(path, mPos) && mPos > 0)
	mPos--;
    mPos = rfindPathSep(path, mPos);
    if(mPos == std::string::npos)
	mPos = moveToStartDir(path);
    return mPos;
    }

size_t FilePathImmutable::moveRightPathSep(OovStringRef const path)
    {
    if(isPathSep(path, mPos) && mPos < path.numChars())
	mPos++;
    mPos = findPathSep(path, mPos);
    if(mPos == std::string::npos)
	mPos = moveToEndDir(path);
    return mPos;
    }

OovString FilePathImmutable::getHead(OovStringRef const path) const
    {
    return std::string(path).substr(0, mPos+1);
    }

OovString FilePathImmutable::getTail(OovStringRef const path) const
    {
    return std::string(path).substr(mPos);
    }

OovString FilePathImmutable::getPathSegment(OovStringRef const path) const
    {
    OovString part;
    size_t startPos = 0;
    if(isPathSep(path, mPos))
	startPos = mPos+1;
    else
	{
	/// @todo - this should go back to find drives
	startPos = rfindPathSep(path, mPos);
	if(startPos == std::string::npos)
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

size_t FilePathImmutable::findPathSegment(OovStringRef const path,
	OovStringRef const seg) const
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
	    if(isPathSep(path, pos-1) &&
		    isPathSep(path, endPos))
		{
		foundPos = pos-1;
		}
	    }
	}
    return foundPos;
    }

OovString FilePathImmutable::getWithoutEndPathSep(OovStringRef const path) const
    {
    OovString str = path;
    removePathSep(str, str.length()-1);
    return str;
    }

OovString FilePathImmutable::getDrivePath(OovStringRef const path) const
    {
    FilePath fp(path, FP_File);
    fp.moveToEndDir();
    return fp.getHead();
    }

OovString FilePathImmutable::getName(OovStringRef const path) const
    {
    FilePath fp(path, FP_File);
    if(fp.moveToEndDir() != std::string::npos)
	fp.discardHead();
    if(fp.moveToExtension() != std::string::npos)
	fp.discardExtension();
    return fp;
    }

OovString FilePathImmutable::getNameExt(OovStringRef const path) const
    {
    FilePath fp(path, FP_File);
    if(fp.moveToEndDir() != std::string::npos)
	fp.discardHead();
    return fp;
    }

OovString FilePathImmutable::getExtension(OovStringRef const path) const
    {
    OovString ext;
    size_t extPos = findExtension(path);
    if(extPos != std::string::npos)
	ext = std::string(path).substr(extPos, strlen(path) - extPos);
    return ext;
    }

bool FilePathImmutable::matchExtension(OovStringRef const path1, OovStringRef const path2) const
    {
    std::string ext1 = getExtension(path1);
    std::string ext2 = getExtension(path2);
    return(ext1.length() > 0 && ext1.compare(ext2) == 0);
    }

bool FilePathImmutable::isEndPathSep(OovStringRef const path)
    {
    int len = std::string(path).length();
    return(isPathSep(path, len-1));
    }

size_t FilePathImmutable::findExtension(OovStringRef const path)
    {
    size_t pathPos = rfindPathSep(path);
    size_t extPos = std::string(path).rfind('.');
    if((pathPos != std::string::npos) && (extPos != std::string::npos) && (extPos < pathPos))
	extPos = std::string::npos;
    return extPos;
    }

bool FilePathImmutable::isDirOnDisk(OovStringRef const path)
    {
    struct OovStat32 statval;
    return((OovStat32(path, &statval) == 0 && S_ISDIR(statval.st_mode)));
    }

bool FilePathImmutable::isAbsolutePath(OovStringRef const path)
    {
    bool absDrv = false;
    size_t drvPos = std::string(path).find(':');
    if(drvPos != std::string::npos)
	{
	absDrv = isPathSep(path, drvPos+1);
	}
    return(isPathSep(path, 0) || absDrv);
    }

int FilePathImmutable::comparePaths(OovStringRef const path1,
	OovStringRef const path2)
    {
#ifdef __linux__
    return strcmp(path1, path2);
#else
    return StringCompareNoCase(path1, path2);
#endif
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
		if(!isEndPathSep(path))
		    dst.append("/");
		}
	    }
	else if(fpt == FP_File)
	    {
	    dst = path;
	    }
	else if(fpt == FP_Ext)
	    {
	    if(!isExtensionSep(path, 0))
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
    else if(isEndPathSep(pathStdStr()))
	fpt = FP_Dir;
    else
	fpt = FP_File;
    return fpt;
    }

void FilePath::appendPathAtPos(OovStringRef const pathPart)
    {
    char const *pp = pathPart;
    if(getPos() != std::string::npos)
	{
	if(isPathSep(pp, 0))
	    pp++;
	if(isPathSep(pathStdStr(), getPos()))
	    pathStdStr().erase(getPos()+1);
	else
	    {
	    pathStdStr().erase(getPos());
	    }
	}
    pathStdStr().append(pp);
    }

void FilePath::appendDirAtPos(OovStringRef const pathPart)
    {
    appendPathAtPos(pathPart);
    if(!isEndPathSep(pathStdStr()))
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
    moveToEndDir();
    appendDirAtPos(pathPart);
    }

void FilePath::discardTail()
    {
    // Keep the end sep so that this is still indicated as a directory.
    if(isPathSep(pathStdStr(), getPos()))
	pathStdStr().erase(getPos()+1);
    else
	pathStdStr().erase(getPos());
    }

void FilePath::appendFile(OovStringRef const fileName)
    {
    moveToEndDir();
    OovString fn = fileName;
    if(isEndPathSep(pathStdStr()))
	{
	removePathSep(fn, 0);
	}
    pathStdStr().append(fn);
    }

void FilePath::appendExtension(OovStringRef const fileName)
    {
    discardExtension();
    if(findExtension(fileName) == std::string::npos)
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
    ensureLastPathSep(pathStdStr());
    }

FilePath FilePath::getParent() const
    {
    FilePath parent(*this);
    parent.moveToEndDir();
    parent.moveLeftPathSep();
    parent.discardTail();
    return parent;
    }

void FilePath::discardDirectory()
    {
    moveToEndDir();
    discardHead();
    }

void FilePath::discardFilename()
    {
    moveToEndDir();
    discardTail();
    }

void FilePath::discardExtension()
    {
    if(moveToExtension() != std::string::npos)
	discardTail();
    }

void FilePath::discardHead()
    { pathStdStr().erase(0, getPos()+1); }

int FilePath::discardLeadingRelSegments()
    {
    int count = 0;
    bool didSeg = true;
    do
	{
	std::string seg = getPathSegment();
	if(seg.compare("..") == 0)
	    {
	    pathStdStr().erase(0, 3);
	    count++;
	    }
	else if(seg.compare(".") == 0)
	    {
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
	pathStdStr().erase(0, part.length());
	}
    }

void FilePath::getAbsolutePath(OovStringRef const path, eFilePathTypes fpt)
    {
    if(isAbsolutePath(path))
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
	moveToEndDir();
	for(int i=0; i<count; i++)
	    {
	    moveLeftPathSep();
	    }
	appendPathAtPos(newPath);
	}
    normalizePathSeps(pathStdStr());
    if(fpt == FP_Dir)
	{
	if(!isEndPathSep(pathStdStr()))
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
