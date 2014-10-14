/*
 * IncludeMap.cpp
 *
 *  Created on: Jan 6, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#include "IncludeMap.h"
#include "Components.h"	// For isHeader
#include <algorithm>
#include <assert.h>

void IncDirDependencyMapReader::read(char const * const fn)
    {
    setFilename(fn);
    readFile();
    }

void discardDirs(std::vector<std::string> &paths)
    {
    for(auto &fn : paths)
	{
	FilePath fp(fn.c_str(), FP_File);
	fn = fp.getNameExt();
	}
    }

std::vector<std::string> IncDirDependencyMapReader::getIncludeFilesDefinedInDirectory(
	char const * const dirName) const
    {
    std::vector<std::string> possibleHeaders = getFilesDefinedInDirectory(dirName);
    std::vector<std::string> headers;
    std::copy_if(possibleHeaders.begin(), possibleHeaders.end(),
	    std::back_inserter(headers),
	    [](std::string const &header) { return isHeader(header.c_str()); });
    return headers;
    }

std::vector<std::string> IncDirDependencyMapReader::getFilesDefinedInDirectory(
	char const * const dirName) const
    {
    std::vector<std::string> incFiles;
    FilePath matchDir(dirName, FP_Dir);
    for(auto const &incDep : getNameValues())
	{
	FilePath incDir(incDep.first.c_str(), FP_File);
	incDir.discardFilename();
	if(matchDir.compare(incDir) == 0)
	    {
	    incFiles.push_back(incDep.first.c_str());
	    }
	}
    return incFiles;
    }

void IncDirDependencyMapReader::getImmediateIncludeFilesUsedBySourceFile(
	char const * const srcName, std::set<IncludedPath> &incFiles) const
    {
    // First check if the includer filename exists in the dependency file.
    FilePath fp(srcName, FP_File);
    std::string val = getValue(fp.c_str());
    if(val.size() > 0)
	{
	CompoundValue compVal;
	compVal.parseString(val.c_str());
	// There are two time values followed by directory/filename pairs.
	if(compVal.size() > 2 && (compVal.size() % 2 == 0))
	    {
	    // Skip first two time values.
	    for(size_t i=2; i<compVal.size(); i+=2)
		{
		IncludedPath incPath(compVal[i], compVal[i+1]);
		incFiles.insert(incPath);
		}
	    }
	else
	    {
	    fprintf(stderr, "Bad entry in oovcde-incdeps.txt:\n   %s\n", val.c_str());
	    assert(false);
	    }
	}
    }

/// This is recursive.
void IncDirDependencyMapReader::getNestedIncludeFilesUsedBySourceFile(
	char const * const srcName, std::set<IncludedPath> &incFiles) const
    {
    // First check if the includer filename exists in the dependency file.
    FilePath fp(srcName, FP_File);
    std::string val = getValue(fp.c_str());
    if(val.size() > 0)
	{
	CompoundValue compVal;
	compVal.parseString(val.c_str());
	// There are two time values followed by directory/filename pairs.
	if(compVal.size() > 2 && (compVal.size() % 2 == 0))
	    {
	    // Skip first two time values.
	    for(size_t i=2; i<compVal.size(); i+=2)
		{
		IncludedPath incPath(compVal[i], compVal[i+1]);
		const auto &ret = incFiles.insert(incPath);
		// Check if this is the first insertion of this value.
		// Prevents infinite recursion and duplicates.
		if(ret.second == true)
		    {
		    getNestedIncludeFilesUsedBySourceFile(incPath.getFullPath().c_str(), incFiles);
		    }
		}
	    }
	else
	    {
	    fprintf(stderr, "Bad entry in oovcde-incdeps.txt:\n   %s\n", val.c_str());
	    assert(false);
	    }
	}
    }

/// This is recursive
std::vector<std::string> IncDirDependencyMapReader::getNestedIncludeDirsUsedBySourceFile(
	char const * const srcName) const
    {
    std::set<IncludedPath> incFiles;
    getNestedIncludeFilesUsedBySourceFile(srcName, incFiles);
    std::set<std::string> tempDirs;
    for(const auto &incFile : incFiles)
	{
	tempDirs.insert(incFile.getIncDir());
	}
    std::vector<std::string> incDirs(tempDirs.size());
    std::copy(tempDirs.begin(), tempDirs.end(), incDirs.begin());
    return incDirs;
    }

/*
std::set<std::string> IncDirDependencyMapReader::getIncludeDirsUsedByDirectory(
	char const * const compDir)
    {
    std::vector<std::string> sources = getFilesDefinedInDirectory(compDir);
    std::set<std::string> includes;
    for(auto const &src : sources)
	{
	std::vector<std::string> srcIncs =
		getIncludeDirsUsedBySourceFile(src.c_str());
	for(auto const &srcInc : srcIncs)
	    {
	    includes.insert(srcInc);
	    }
	}
    return includes;
    }
*/

#define FAST_MATCH 1
#if(FAST_MATCH)
/// This is recursive
bool IncDirDependencyMapReader::anyRootDirsUsedBySourceFile(
	std::vector<std::string> const &incRoots,
	std::set<std::string> &includesUsedSoFar,
	char const * const srcName
	) const
    {
    bool match = false;
    std::set<IncludedPath> incFiles;
    getImmediateIncludeFilesUsedBySourceFile(srcName, incFiles);
    for(const auto &incFile : incFiles)
	{
	auto const insertRet = includesUsedSoFar.insert(incFile.getFullPath());
	if(insertRet.second)
	    {
	    for(auto const &root: incRoots)
		{
		if(root.find(incFile.getIncDir()) != std::string::npos)
		    {
		    match = true;
		    break;
		    }
		}
	    if(match)
		break;
	    match = anyRootDirsUsedBySourceFile(incRoots, includesUsedSoFar,
		    incFile.getFullPath().c_str());
	    if(match)
		break;
	    }
	}
    return match;
    }
#endif

bool IncDirDependencyMapReader::anyRootDirsMatch(std::vector<std::string> const &incRoots,
	char const * const dirName) const
    {
    bool match = false;
    std::set<std::string> includesUsedSoFar;
    std::vector<std::string> sources = getFilesDefinedInDirectory(dirName);
    for(auto const &src : sources)
	{
#if(FAST_MATCH)
	match = anyRootDirsUsedBySourceFile(incRoots, includesUsedSoFar, src.c_str());
#else
	std::vector<std::string> srcIncDirs =
		getNestedIncludeDirsUsedBySourceFile(src.c_str());
	for(auto const &srcIncDir : srcIncDirs)
	    {
	    auto const insertRet = includesUsedSoFar.insert(srcIncDir);
	    if(insertRet.second)
		{
		match = (std::find(incRoots.begin(), incRoots.end(),
			srcIncDir) != incRoots.end());
		}
	    if(match)
		break;
	    }
#endif
	if(match)
	    break;
	}
    return match;
    }

std::vector<std::string> IncDirDependencyMapReader::getOrderedIncludeDirsForSourceFile(char const * const absSrcName,
	const std::vector<std::string> &orderedIncRoots) const
    {
    std::vector<std::string> incDirs;
    // put directories in search path order.
    struct DirInfo
    {
	DirInfo():
	    mMatchLen(0), mMatchIndex(-1)
	    {}
	size_t mMatchLen;
	size_t mMatchIndex;
    };
    std::vector<DirInfo> unorderedDirInfo;
    std::vector<std::string> unorderedIncDirs =
	    getNestedIncludeDirsUsedBySourceFile(absSrcName);
    unorderedDirInfo.resize(unorderedIncDirs.size());
    // Go through all directories and for each, find the longest root directory that matches.
    for(size_t incDirI = 0; incDirI < unorderedIncDirs.size(); incDirI++)
	{
	auto &incInfo = unorderedDirInfo[incDirI];
	for(size_t incRootI = 0; incRootI < orderedIncRoots.size(); incRootI++)
	    {
	    auto const &incRoot = orderedIncRoots[incRootI];
	    if(unorderedIncDirs[incDirI].find(incRoot) != std::string::npos)
		{
		if(incRoot.length() > incInfo.mMatchLen)
		    {
		    incInfo.mMatchLen = incRoot.length();
		    incInfo.mMatchIndex = incRootI;
		    }
		}
	    }
#if DEBUG_FINDER
	if(incInfo.mMatchLen == 0)
	    {
	    for(auto const incRoot : orderedIncRoots)
		fprintf(sLog.mFp, "incRoot = %s\n", incRoot.c_str());
	    for(auto const incDir : unorderedIncDirs)
		fprintf(sLog.mFp, "unord = %s\n", incDir.c_str());
	    }
#endif
	}
    for(size_t incRootI = 0; incRootI < orderedIncRoots.size(); incRootI++)
	{
	for(size_t incDirI = 0; incDirI < unorderedIncDirs.size(); incDirI++)
	    {
	    auto &incInfo = unorderedDirInfo[incDirI];
	    if(incInfo.mMatchIndex == incRootI)
		{
		incDirs.push_back(unorderedIncDirs[incDirI]);
		}
	    }
	}
    return incDirs;
    }

