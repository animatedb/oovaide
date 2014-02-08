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
    if(!FilePath::isEndPathSep(path.c_str()))
	path += '/';
    }

bool ensurePathExists(char const * const path)
    {
    bool success = true;

    // Walk up the tree to find a base that exists.
    FilePath fp(path, FP_File);
    size_t pos = fp.moveToEndDir();
    while(pos != std::string::npos)
	{
	fp.discardTail();
	if(fileExists(fp.c_str()))
	    break;
	else
	    pos = fp.moveLeftPathSep();
	}
    while(pos != std::string::npos && success)
	{
	pos = findPathSep(path, pos);
	if(pos != std::string::npos)
	    {
	    std::string partPath = path;
	    partPath.resize(pos);
	    if(!fileExists(partPath.c_str()))
		{
#ifdef __linux__
		success = (mkdir(partPath.c_str(), 0x1FF) == 0);	// 0777
#else
		success = (_mkdir(partPath.c_str()) == 0);
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

size_t findPathSep(char const * const path, size_t startPos)
    {
    const std::string str = path;
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

size_t rfindPathSep(char const * const path, size_t startPos)
    {
    const std::string str = path;
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

bool fileExists(char const * const path)
    {
    std::string tempPath = path;
    removePathSep(tempPath, tempPath.size()-1);
    struct OovStat32 statval;
    return(OovStat32(tempPath.c_str(), &statval) == 0);
    }

void deleteFile(char const * const path)
    {
    unlink(path);
    }

void renameFile(char const * const oldPath, char const * const newPath)
    {
    rename(oldPath, newPath);
    }

bool getFileTime(char const * const path, time_t &time)
    {
    struct OovStat32 srcFileStat;
    bool success = (OovStat32(path, &srcFileStat) == 0);
    if(success)
	time = srcFileStat.st_mtime;
    return success;
    }

bool anyExtensionMatch(FilePaths const &paths, char const * const file)
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

size_t FilePathImmutableBase::moveToStartDir(char const * const path)
    {
    mPos = std::string(path).rfind(':');
    if(mPos == std::string::npos)
	mPos = 0;
    return mPos;
    }

size_t FilePathImmutableBase::moveToEndDir(char const * const path)
    {
    size_t pos;
    if(isEndPathSep(path))
	{
	pos = strlen(path) - 1;
	mPos = pos;
	}
    else
	{
	pos = rfindPathSep(path);
	if(pos == std::string::npos)
	    mPos = strlen(path) - 1;
	else
	    mPos = pos;
	}
    return pos;
    }

size_t FilePathImmutableBase::moveToExtension(char const * const path)
    {
    size_t pos = findExtension(path);
    if(pos == std::string::npos)
	mPos = 0;
    else
	mPos = pos;
    return pos;
    }

size_t FilePathImmutableBase::moveLeftPathSep(char const * const path)
    {
    if(isPathSep(path, mPos) && mPos > 0)
	mPos--;
    mPos = rfindPathSep(path, mPos);
    if(mPos == std::string::npos)
	mPos = moveToStartDir(path);
    return mPos;
    }

std::string FilePathImmutableBase::getHead(char const * const path) const
    {
    std::string str = std::string(path).substr(0, mPos+1);
    return str;
    }

std::string FilePathImmutableBase::getTail(char const * const path) const
    {
    std::string str = std::string(path).substr(mPos);
    return str;
    }

std::string FilePathImmutableBase::getPathSegment(char const * const path) const
    {
    std::string part;
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

size_t FilePathImmutableBase::findPathSegment(char const * const path,
	char const * const seg) const
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
    size_t pos = lowerPath.find(lowerSeg.c_str());
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

std::string FilePathImmutableBase::getWithoutEndPathSep(char const * const path) const
    {
    std::string str = path;
    removePathSep(str, str.length()-1);
    return str;
    }

std::string FilePathImmutableBase::getDrivePath(char const * const path) const
    {
    FilePath fp(path, FP_File);
    fp.moveToEndDir();
    return fp.getHead();
    }

std::string FilePathImmutableBase::getName(char const * const path) const
    {
    FilePath fp(path, FP_File);
    if(fp.moveToEndDir() != std::string::npos)
	fp.discardHead();
    if(fp.moveToExtension() != std::string::npos)
	fp.discardExtension();
    return fp;
    }

std::string FilePathImmutableBase::getNameExt(char const * const path) const
    {
    FilePath fp(path, FP_File);
    if(fp.moveToEndDir() != std::string::npos)
	fp.discardHead();
    return fp;
    }

std::string FilePathImmutableBase::getExtension(char const * const path) const
    {
    std::string ext;
    size_t extPos = findExtension(path);
    if(extPos != std::string::npos)
	ext = std::string(path).substr(extPos, strlen(path) - extPos);
    return ext;
    }

bool FilePathImmutableBase::matchExtension(char const * const path1, char const * const path2) const
    {
    std::string ext1 = getExtension(path1);
    std::string ext2 = getExtension(path2);
    return(ext1.length() > 0 && ext1.compare(ext2) == 0);
    }

bool FilePathImmutableBase::isEndPathSep(char const * const path)
    {
    int len = std::string(path).length();
    return(isPathSep(path, len-1));
    }

size_t FilePathImmutableBase::findExtension(char const * const path)
    {
    size_t pathPos = rfindPathSep(path);
    size_t extPos = std::string(path).rfind('.');
    if((pathPos != std::string::npos) && (extPos != std::string::npos) && (extPos < pathPos))
	extPos = std::string::npos;
    return extPos;
    }

bool FilePathImmutableBase::isDirOnDisk(char const * const path)
    {
    struct OovStat32 statval;
    return((OovStat32(path, &statval) == 0 && S_ISDIR(statval.st_mode)));
    }

bool FilePathImmutableBase::isAbsolutePath(char const * const path)
    {
    bool absDrv = false;
    size_t drvPos = std::string(path).find(':');
    if(drvPos != std::string::npos)
	{
	absDrv = isPathSep(path, drvPos+1);
	}
    return(isPathSep(path, 0) || absDrv);
    }


///////////

std::string FilePathRef::normalizePathType(char const * const path, eFilePathTypes fpt)
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

eFilePathTypes FilePathRef::getType() const
    {
    eFilePathTypes fpt;
    if(pathStdStr()[0] == '.')
	fpt = FP_Ext;
    else if(isEndPathSep(pathStdStr().c_str()))
	fpt = FP_Dir;
    else
	fpt = FP_File;
    return fpt;
    }

void FilePathRef::appendPathAtPos(char const * const pathPart)
    {
    char const *pp = pathPart;
    if(getPos() != std::string::npos)
	{
	if(isPathSep(pp, 0))
	    pp++;
	if(isPathSep(pathStdStr().c_str(), getPos()))
	    pathStdStr().erase(getPos()+1);
	else
	    {
	    pathStdStr().erase(getPos());
	    }
	}
    pathStdStr().append(pp);
    }

void FilePathRef::appendDirAtPos(char const * const pathPart)
    {
    appendPathAtPos(pathPart);
    if(!isEndPathSep(pathStdStr().c_str()))
	pathStdStr().append("/");
    }

void FilePathRef::appendPart(char const * const pathPart, eFilePathTypes fpt)
    {
    if(fpt == FP_Dir)
	appendDir(pathPart);
    else
	appendFile(pathPart);
    }

void FilePathRef::appendDir(char const * const pathPart)
    {
    moveToEndDir();
    appendDirAtPos(pathPart);
    }

void FilePathRef::discardTail()
    {
    // Keep the end sep so that this is still indicated as a directory.
    if(isPathSep(pathStdStr().c_str(), getPos()))
	pathStdStr().erase(getPos()+1);
    else
	pathStdStr().erase(getPos());
    }

void FilePathRef::appendFile(char const * const fileName)
    {
    moveToEndDir();
    std::string fn = fileName;
    if(isEndPathSep(pathStdStr().c_str()))
	{
	removePathSep(fn, 0);
	}
    pathStdStr().append(fn);
    }

void FilePathRef::appendExtension(char const * const fileName)
    {
    discardExtension();
    if(findExtension(fileName) == std::string::npos)
	pathStdStr().append(".");
    pathStdStr().append(fileName);
    }

void FilePathRef::getWorkingDirectory()
    {
    char buf[1000];
    pathStdStr().clear();
    if(getcwd(buf, sizeof(buf)))
	{
	pathStdStr().append(buf);
	}
    ensureLastPathSep(pathStdStr());
    }

void FilePathRef::discardDirectory()
    {
    moveToEndDir();
    discardHead();
    }

void FilePathRef::discardFilename()
    {
    moveToEndDir();
    discardTail();
    }

void FilePathRef::discardExtension()
    {
    if(moveToExtension() != std::string::npos)
	discardTail();
    }

void FilePathRef::discardHead()
    { pathStdStr().erase(0, getPos()+1); }

int FilePathRef::discardLeadingRelSegments()
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

void FilePathRef::discardMatchingHead(char const * const pathPart)
    {
    std::string part(pathPart);
    if(pathStdStr().compare(0, part.length(), part) == 0)
	{
	pathStdStr().erase(0, part.length());
	}
    }

void FilePathRef::getAbsolutePath(char const * const path, eFilePathTypes fpt)
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
	appendPathAtPos(newPath.c_str());
	}
    normalizePathSeps(pathStdStr());
    if(fpt == FP_Dir)
	{
	if(!isEndPathSep(pathStdStr().c_str()))
	    pathStdStr().append("/");
	}
    }

void FilePathRef::normalizePathSeps(std::string &path)
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
